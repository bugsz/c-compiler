package main

import (
	"bytes"
	"encoding/json"
	"errors"
	"io"
	"net/http"
	"os"
	"os/exec"
	"sync"
	"time"

	"github.com/gin-gonic/gin"
	"github.com/gofrs/uuid"
)

var definitions int

type AST struct {
	Id       string `json:"id"`
	Label    string `json:"label"`
	Position string `json:"position"`
	Val      string `json:"val"`
	Ctype    string `json:"ctype"`
	Type     string `json:"type"`
	Children []AST  `json:"children"`
}

type LimitedBuffer struct {
	b              bytes.Buffer
	maxSize        int
	bufferTooLarge chan struct{}
}

func (buf *LimitedBuffer) Read(p []byte) (n int, err error) {
	return buf.b.Read(p)
}
func (buf *LimitedBuffer) Write(p []byte) (n int, err error) {
	defer func() {
		if buf.b.Len() > buf.maxSize {
			buf.bufferTooLarge <- struct{}{}
		}
	}()
	return buf.b.Write(p)
}

type ThreadSafeBuffer struct {
	b  bytes.Buffer
	rw sync.RWMutex
}

func (buf *ThreadSafeBuffer) Read(p []byte) (n int, err error) {
	buf.rw.RLock()
	defer buf.rw.RUnlock()
	return buf.b.Read(p)
}
func (buf *ThreadSafeBuffer) Write(p []byte) (n int, err error) {
	buf.rw.Lock()
	defer buf.rw.Unlock()
	return buf.b.Write(p)
}

type RunCodeRequest struct {
	Code  string `json:"code"`
	Input string `json:"input"`
}

func Cors() gin.HandlerFunc {
	return func(context *gin.Context) {
		method := context.Request.Method
		_, ok := context.RemoteIP()
		if ok {
			context.Header("Access-Control-Allow-Origin", "*")
			context.Header("Access-Control-Allow-Headers", "Content-Type,AccessToken,X-CSRF-Token, Authorization, Token")
			context.Header("Access-Control-Allow-Methods", "POST, GET, OPTIONS")
			context.Header("Access-Control-Expose-Headers", "Content-Length, Access-Control-Allow-Origin, Access-Control-Allow-Headers, Content-Type")
			context.Header("Access-Control-Allow-Credentials", "true")
		} else {
			context.AbortWithStatus(http.StatusForbidden)
		}
		if method == "OPTIONS" {
			context.AbortWithStatus(http.StatusNoContent)
		}
		context.Next()
	}
}

func main() {
	ticker := time.NewTicker(time.Second * 60)
	go func() {
		for {
			<-ticker.C
			defaultAST := &AST{}
			cmd := exec.Command("./serializer", "-f", "builtin.h")
			bytes, _ := cmd.Output()
			json.Unmarshal(bytes, defaultAST)
			definitions = len(defaultAST.Children)
		}
	}()
	r := gin.Default()
	r.SetTrustedProxies([]string{"127.0.0.1"})
	r.Use(Cors())
	r.POST("/dumpAST", func(c *gin.Context) {
		buffer := new(bytes.Buffer)
		cmd := exec.Command("./serializer", "-stdin", "-ast-dump")
		cmd.Stdin = c.Request.Body
		cmd.Stderr = buffer
		bytes, err := cmd.Output()
		if err != nil {
			c.Data(400, "plaintext", buffer.Bytes())
			return
		}
		c.Data(200, "application/json", bytes)
	})

	r.POST("/GetAST", func(c *gin.Context) {
		buffer := new(bytes.Buffer)
		tmp_file, _ := os.CreateTemp("", "tmp_")
		io.Copy(tmp_file, c.Request.Body)
		cmd := exec.Command("./serializer", "-f", tmp_file.Name())
		cmd.Stderr = buffer
		bytes, err := cmd.Output()
		if err != nil {
			c.Data(400, "plaintext", buffer.Bytes())
			return
		}
		os.Remove(tmp_file.Name())
		curAST := &AST{}
		json.Unmarshal(bytes, curAST)
		curAST.Children = curAST.Children[definitions:]
		c.JSON(200, curAST)
	})

	r.POST("/GetRunningResult", func(c *gin.Context) {
		var req RunCodeRequest
		c.BindJSON(&req)
		tmp_file, _ := os.CreateTemp("", "tmp_")
		io.Copy(tmp_file, c.Request.Body)
		genIR := exec.Command("./llvm_wrapper", "-f", tmp_file.Name())
		buffer := new(ThreadSafeBuffer)
		genIR.Stderr = buffer
		filename := uuid.Must(uuid.NewV4()).String()
		llvmAs := exec.Command("llvm-as", "-o", filename)
		llvmAs.Stdin, _ = genIR.StdoutPipe()
		llvmAs.Stderr = buffer
		genIR.Start()
		_, err := llvmAs.Output()
		genIR.Wait()
		if err != nil {
			generr := []byte("Fail to generate execuable file\n")
			stderr := append([]byte("\nstderr:\n"), buffer.b.Bytes()...)
			errMsg := append(append(generr, stderr...), err.Error()...)
			c.Data(400, "plaintext", errMsg)
			os.Remove(tmp_file.Name())
			return
		}
		os.Remove(tmp_file.Name())
		workDone := make(chan struct{}, 1)
		bufferTooLarge := make(chan struct{}, 1)
		stdout := &LimitedBuffer{maxSize: 5000, bufferTooLarge: bufferTooLarge}
		llvmJIT := exec.Command("lli", filename)
		llvmJIT.Stdin = bytes.NewReader([]byte(req.Input))
		llvmJIT.Stdout = stdout
		llvmJIT.Stderr = buffer
		go func() {
			err = llvmJIT.Run()
			workDone <- struct{}{}
		}()
		select {
		case <-time.After(5 * time.Second):
			err = errors.New("error: Time Limit Exceeded")
			llvmJIT.Process.Kill()
		case <-bufferTooLarge:
			err = errors.New("error: Output Limit Exceeded")
			llvmJIT.Process.Kill()
		case <-workDone:
		}
		os.Remove(filename)
		if err != nil {
			stdout := append([]byte("stdout:\n"), stdout.b.Bytes()...)
			stderr := append([]byte("\nstderr:\n"), buffer.b.Bytes()...)
			errMsg := append(append(stdout, stderr...), err.Error()...)
			c.Data(400, "plaintext", errMsg)
			return
		}
		c.Data(200, "plaintext", stdout.b.Bytes())
	})

	r.POST("/GetIR", func(c *gin.Context) {
		buffer := new(bytes.Buffer)
		genIR := exec.Command("./llvm_wrapper", "-stdin")
		genIR.Stdin = c.Request.Body
		genIR.Stderr = buffer
		bytes, err := genIR.Output()
		if err != nil {
			c.Data(400, "plaintext", buffer.Bytes())
			return
		}
		c.Data(200, "plaintext", bytes)
	})

	r.POST("/deploy", func(c *gin.Context) {
		c.AbortWithStatus(200)
		go update()
	})

	r.Run("127.0.0.1:8080")
}

func update() {
	f, err := os.OpenFile("update.log", os.O_CREATE|os.O_APPEND|os.O_RDWR, os.ModePerm)
	if err != nil {
		return
	}
	now := time.Now()
	f.WriteString(now.Format("[UPDATE BEGIN AT] 2006-01-02 15:04:05.000 \n"))
	defer func() {
		if err != nil {
			f.WriteString(now.Format("[UPDATE FAILED] 2006-01-02 15:04:05.000 \n"))
		} else {
			f.WriteString(now.Format("[UPDATE FINISHED AT] 2006-01-02 15:04:05.000 \n"))
		}
		f.Close()
	}()
	updateProject := exec.Command("sh", "./update.sh")
	updateProject.Stdout = f
	updateProject.Stderr = f
	err = updateProject.Run()
}
