###
 # @Author: Pan Zhiyuan
 # @Date: 2022-04-14 16:57:11
 # @LastEditors: Pan Zhiyuan
 # @FilePath: /project/src/frontend/case-test.sh
 # @Description: 
### 

# iterate through all C files in test folder
for file in $(find test -name "*.c")
do
    ./zjucc -f $file --ast-dump --sym-dump
done