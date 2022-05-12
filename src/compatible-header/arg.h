#ifndef __ARG_H
#define __ARG_H

#ifndef _VA_LIST
typedef __builtin_va_list va_list;
// struct __va_list_tag {
//     unsigned int gp_offset;   size 4
//     unsigned int fp_offset;   size 4
//     void *overflow_arg_area;  size 8
//     void *reg_save_area;      size 8
// }
#define _VA_LIST
#endif
#define va_start(ap, param) __llvm_va_start(ap, param)
#define va_end(ap)          __llvm_va_end(ap)

/*
Since every x64 register is 64 bit,=
and 6 of them can be used to pass params,=
so the offset is at most 40(may equal)=
*/  
#define va_arg(ap, type)\
int gp_offset = *(int *)ap;\
char * overflow_arg_area = *(long *)(ap+8);\
char * reg_save_area = *(long *)(ap+16);\
char buf[sizeof(type)];\
int size = sizeof(type);\
if(gp_offset <= 40){\
    memcpy(buf, reg_save_area, size);\
    *(int *)ap += size;\ 
    *(long *)(ap+16) += size;\
}else{\
    memcpy(buf, overflow_arg_area, size);\
        *(long *)(ap+8) += size;\
}\
*(type *)buf;\

#endif /* __ARG_H */