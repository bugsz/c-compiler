package main

import (
	"bytes"
	"net/http"
	"os/exec"

	"github.com/gin-gonic/gin"
)

func Cors() gin.HandlerFunc {
	return func(context *gin.Context) {
		method := context.Request.Method
		context.Header("Access-Control-Allow-Origin", "*")
		context.Header("Access-Control-Allow-Headers", "Content-Type,AccessToken,X-CSRF-Token, Authorization, Token")
		context.Header("Access-Control-Allow-Methods", "POST, GET, OPTIONS")
		context.Header("Access-Control-Expose-Headers", "Content-Length, Access-Control-Allow-Origin, Access-Control-Allow-Headers, Content-Type")
		context.Header("Access-Control-Allow-Credentials", "true")
		if method == "OPTIONS" {
			context.AbortWithStatus(http.StatusNoContent)
		}
		context.Next()
	}
}

func main() {
	r := gin.Default()
	r.Use(Cors())
	r.POST("/GetAST", func(c *gin.Context) {
		buffer := new(bytes.Buffer)
		cmd := exec.Command("./bin/serializer", "-stdin")
		cmd.Stdin = c.Request.Body
		cmd.Stderr = buffer
		bytes, err := cmd.Output()
		if err != nil {
			c.Data(400, "plaintext", buffer.Bytes())
		}
		c.Data(200, "application/json", bytes)
	})

	r.POST("/GetRunningResult", func(c *gin.Context) {
		c.String(200, "TO BE DONE")
	})
	r.Run() // listen and serve on 0.0.0.0:8080
}
