#ifndef __ARMv7M_LOCK_H__
#define __ARMv7M_LOCK_H__

#define __ldrex(addr)				({	\
	unsigned int __result = 0;			\
	__asm__ __volatile__("ldrex %0, [%1]"		\
		: "=r"(__result)			\
		: "r"(addr)				\
		: "memory");				\
	__result;					\
})

#define __strex(value, addr)			({	\
	unsigned int __result = 0;			\
	__asm__ __volatile__("strex %0, %2, [%1]"	\
		: "=r"(result)				\
		: "r"(addr), "r"(value)			\
		: "memory");				\
	__result;					\
})

/* return 0 if success */
static inline int atomic_set(int *p, int v)
{
	int result;

	/* dmb() */
	__ldrex(p);
	/* dmb() */
	result = __strex(v, p);

	return result;
}

#endif /* __ARMv7M_LOCK_H__ */