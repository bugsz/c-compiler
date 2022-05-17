#undef assert
#undef __assert

#ifdef NDEBUG
#define	assert(e)	(0)
#else

#ifdef __FILE_NAME__
#define __ASSERT_FILE_NAME __FILE_NAME__
#else /* __FILE_NAME__ */
#define __ASSERT_FILE_NAME __FILE__
#endif /* __FILE_NAME__ */

#ifndef _ASSERT_H_
#define _ASSERT_H_

void __assert_fail(char *__assertion, char *__file, int __line, char *__func);
void __assert_rtn(char *__func, char *__file, int __line, char *__assertion);

#ifdef __Darwin__
#define __assert(e, file, line, func) __assert_rtn(func, file, line, e)
#else
#define __assert(e, file, line, func) __assert_fail(e, file, line, func)
#endif

#define assert(expr)                                                \
  ({if((expr) == 0)                                                 \
    __assert(#expr, __FILE__, __LINE__, (char *) 0);                \
  })                                                                \

#endif
#endif