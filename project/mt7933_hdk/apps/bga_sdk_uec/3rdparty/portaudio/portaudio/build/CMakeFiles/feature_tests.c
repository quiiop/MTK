
const char features[] = {"\n"
                         "C_FEATURE:"
#if (__GNUC__ * 100 + __GNUC_MINOR__) >= 304
                         "1"
#else /* #if (__GNUC__ * 100 + __GNUC_MINOR__) >= 304 */
                         "0"
#endif /* #if (__GNUC__ * 100 + __GNUC_MINOR__) >= 304 */
                         "c_function_prototypes\n"
                         "C_FEATURE:"
#if (__GNUC__ * 100 + __GNUC_MINOR__) >= 304 && defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
                         "1"
#else /* #if (__GNUC__ * 100 + __GNUC_MINOR__) >= 304 && defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L */
                         "0"
#endif /* #if (__GNUC__ * 100 + __GNUC_MINOR__) >= 304 && defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L */
                         "c_restrict\n"
                         "C_FEATURE:"
#if (__GNUC__ * 100 + __GNUC_MINOR__) >= 406 && defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201000L
                         "1"
#else /* #if (__GNUC__ * 100 + __GNUC_MINOR__) >= 406 && defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201000L */
                         "0"
#endif /* #if (__GNUC__ * 100 + __GNUC_MINOR__) >= 406 && defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201000L */
                         "c_static_assert\n"
                         "C_FEATURE:"
#if (__GNUC__ * 100 + __GNUC_MINOR__) >= 304 && defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
                         "1"
#else /* #if (__GNUC__ * 100 + __GNUC_MINOR__) >= 304 && defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L */
                         "0"
#endif /* #if (__GNUC__ * 100 + __GNUC_MINOR__) >= 304 && defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L */
                         "c_variadic_macros\n"

                        };

int main(int argc, char **argv)
{
    (void)argv;
    return features[argc];
}
