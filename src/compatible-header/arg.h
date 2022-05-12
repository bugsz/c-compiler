#ifndef __ARG_H
#define __ARG_H

#ifndef _VA_LIST
typedef char* va_list;
/*
Actually it is
struct __va_list_tag {
    unsigned int gp_offset;   size 4
    unsigned int fp_offset;   size 4
    void *overflow_arg_area;  size 8
    void *reg_save_area;      size 8
}
*/
#define _VA_LIST
#endif

#define va_start(ap, param) {ap = malloc(24);__llvm_va_start(ap);}
#define va_end(ap)          {free(ap);}
void __llvm_va_start(char* ap);

/*
Since every x64 register is 64 bit,
and 6 of them can be used to pass params,
so the offset is at most 40(may equal)
*/  
#define va_arg(ap, type)                                                        \
({                                                                              \
    char buf[8];                                                                \
    int * gp_offset = ap;                                                       \
    int * fp_offset = ap+4;                                                     \
    char * overflow_arg_area = (*(long *)(ap+8));                               \
    char * reg_save_area = (*(long *)(ap+16)) + *gp_offset;                     \
    char * fp_reg_save_area = (*(long *)(ap+16)) + *fp_offset;                  \
    if(isfp(type) && *fp_offset <= 160){                                        \
        memcpy(buf, fp_reg_save_area, sizeof(type));                            \
        *fp_offset += 16;                                                       \
    }else if(!isfp(type) && *gp_offset <= 40){                                  \
        memcpy(buf, reg_save_area, sizeof(type));                               \
        *gp_offset += 8;                                                        \
    }else{                                                                      \
        memcpy(buf, overflow_arg_area, sizeof(type));                           \
        *(long *)(ap+8) += 8;                                                   \
    };                                                                          \
    *(type *)buf;                                                               \
})

#endif /* __ARG_H */