package main

import (
	"bytes"
	"net/http"
	"os"
	"os/exec"
	"sync"
	"time"

	"github.com/gin-gonic/gin"
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
	r := gin.Default()
	r.SetTrustedProxies([]string{"127.0.0.1"})
	r.Use(Cors())
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
		buffer := new(Buffer)
		genIR := exec.Command("./llvm_wrapper", "-stdin")
		genIR.Stdin = c.Request.Body
		genIR.Stderr = buffer
		llvmJIT := exec.Command("lli")
		llvmJIT.Stdin, _ = genIR.StdoutPipe()
		llvmJIT.Stderr = buffer
		genIR.Start()
		bytes, err := llvmJIT.Output()
		if err != nil {
			c.Data(400, "plaintext", buffer.b.Bytes())
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
		c.AbortWithStatus(http.StatusNoContent)
	})

	r.Run("127.0.0.1:8080")
}
