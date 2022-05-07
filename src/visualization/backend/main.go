package main

import (
	"bytes"
	"fmt"
	"net/http"
	"os"
	"os/exec"
	"sync"
	"time"

	"github.com/gin-gonic/gin"
	"github.com/gofrs/uuid"
)

type Buffer struct {
	b  bytes.Buffer
	rw sync.RWMutex
}

func (b *Buffer) Read(p []byte) (n int, err error) {
	b.rw.RLock()
	defer b.rw.RUnlock()
	return b.b.Read(p)
}
func (b *Buffer) Write(p []byte) (n int, err error) {
	b.rw.Lock()
	defer b.rw.Unlock()
	return b.b.Write(p)
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
	ticker := time.NewTicker(time.Second * 5)

	go func() {
		for {
			<-ticker.C
			now := time.Now()
			fmt.Print(now.Format("[CLEAR RUNNING PROCESS] 2006-01-02 15:04:05.000 \n"))
			cmd := exec.Command("killall", "lli")
			buffer := new(Buffer)
			cmd.Stderr = buffer
			_, err := cmd.Output()
			if err != nil {
				fmt.Println(buffer.b.String())
			}
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
		cmd := exec.Command("./serializer", "-stdin")
		cmd.Stdin = c.Request.Body
		cmd.Stderr = buffer
		bytes, err := cmd.Output()
		if err != nil {
			c.Data(400, "plaintext", buffer.Bytes())
			return
		}
		c.Data(200, "application/json", bytes)
	})

	r.POST("/GetRunningResult", func(c *gin.Context) {
		var req RunCodeRequest
		c.BindJSON(&req)
		buffer := new(Buffer)
		genIR := exec.Command("./llvm_wrapper", "-stdin")
		genIR.Stdin = bytes.NewReader([]byte(req.Code))
		genIR.Stderr = buffer
		filename := uuid.Must(uuid.NewV4()).String()
		llvmAs := exec.Command("llvm-as", "-o", filename)
		llvmAs.Stdin, _ = genIR.StdoutPipe()
		llvmAs.Stderr = buffer
		genIR.Start()
		_, err := llvmAs.Output()

		if err != nil {
			generr := []byte("Fail to generate execuable file\n")
			stderr := append([]byte("\nstderr:\n"), buffer.b.Bytes()...)
			errMsg := append(append(generr, stderr...), err.Error()...)
			c.Data(400, "plaintext", errMsg)
			return
		}

		llvmJIT := exec.Command("lli", filename)
		llvmJIT.Stdin = bytes.NewReader([]byte(req.Input))
		llvmJIT.Stderr = buffer
		genIR.Start()
		bytes, err := llvmJIT.Output()
		if err != nil {
			stdout := append([]byte("stdout:\n"), bytes...)
			stderr := append([]byte("\nstderr:\n"), buffer.b.Bytes()...)
			errMsg := append(append(stdout, stderr...), err.Error()...)
			c.Data(400, "plaintext", errMsg)
			return
		}
		c.Data(200, "plaintext", bytes)
	})

	r.POST("/GetIR", func(c *gin.Context) {
		buffer := new(Buffer)
		genIR := exec.Command("./llvm_wrapper", "-stdin")
		genIR.Stdin = c.Request.Body
		genIR.Stderr = buffer
		bytes, err := genIR.Output()
		if err != nil {
			c.Data(400, "plaintext", buffer.b.Bytes())
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
