#pragma once

#ifdef DPDKWIN_NO_WARNINGS
#pragma warning (disable : 2330)
#endif

static inline void rte_prefetch0(const volatile void *p)
{
	_mm_prefetch(p, _MM_HINT_T0);
}

static inline void rte_prefetch1(const volatile void *p)
{
	_mm_prefetch(p, _MM_HINT_T1);
}

static inline void rte_prefetch2(const volatile void *p)
{
	_mm_prefetch(p, _MM_HINT_T2);
}

static inline void rte_prefetch_non_temporal(const volatile void *p)
{
	_mm_prefetch(p, _MM_HINT_NTA);
}

#ifdef DPDKWIN_NO_WARNINGS
#pragma warning (enable : 2330)
#endif
