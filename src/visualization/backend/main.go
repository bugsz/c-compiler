package main

import (
	"os/exec"

	"github.com/gin-gonic/gin"
)

func main() {
	r := gin.Default()
	r.POST("/GetAst", func(c *gin.Context) {
		cmd := exec.Command("./bin/serializer", "-stdin")
		cmd.Stdin = c.Request.Body
		bytes, err := cmd.Output()
		if err != nil {
			c.Error(err)
		}
		c.Data(200, "application/json", bytes)
	})

	r.POST("/GetRunningResult", func(c *gin.Context) {
		c.String(200, "TO BE DONE")
	})
	r.Run() // listen and serve on 0.0.0.0:8080
}
