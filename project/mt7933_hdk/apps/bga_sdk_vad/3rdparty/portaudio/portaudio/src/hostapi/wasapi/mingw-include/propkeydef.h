#ifndef PID_FIRST_USABLE
#define PID_FIRST_USABLE 2
#endif /* #ifndef PID_FIRST_USABLE */

#ifndef REFPROPERTYKEY
#ifdef __cplusplus
#define REFPROPERTYKEY const PROPERTYKEY &
#else /* #ifdef __cplusplus */
#define REFPROPERTYKEY const PROPERTYKEY * __MIDL_CONST
#endif /* #ifdef __cplusplus */
#endif /* #ifndef REFPROPERTYKEY */

#ifdef DEFINE_PROPERTYKEY
#undef DEFINE_PROPERTYKEY
#endif /* #ifdef DEFINE_PROPERTYKEY */

#ifdef INITGUID
#define DEFINE_PROPERTYKEY(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8, pid) EXTERN_C const PROPERTYKEY DECLSPEC_SELECTANY name = { { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }, pid }
#else /* #ifdef INITGUID */
#define DEFINE_PROPERTYKEY(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8, pid) EXTERN_C const PROPERTYKEY name
#endif /* #ifdef INITGUID */

#ifndef IsEqualPropertyKey
#define IsEqualPropertyKey(a, b)   (((a).pid == (b).pid) && IsEqualIID((a).fmtid, (b).fmtid) )
#endif /* #ifndef IsEqualPropertyKey */

