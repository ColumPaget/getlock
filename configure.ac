AC_INIT
AC_CONFIG_SRCDIR([main.c])


AC_PROG_CC 
AC_LANG([C]) 
AC_PROG_MAKE_SET 
AC_PROG_INSTALL 
AC_PROG_EGREP
 
AC_SYS_LARGEFILE



dnl define a function we can use to check if a compiler flag
dnl is supported by the compiler
AC_DEFUN([CHECK_COMPILER_OPT],
[
  COMP_OPT="$@"
  AC_MSG_CHECKING([if compiler supports $COMP_OPT])
  OLD_CFLAGS="$CFLAGS"
  CFLAGS="$CFLAGS $COMP_OPT"
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM(int test() {
    return 1;
  })], [COMP_OPT_SUPPORT="yes"], [COMP_OPT_SUPPORT="no"])
  AC_MSG_RESULT([$COMP_OPT_SUPPORT])
  AS_IF([test "$COMP_OPT_SUPPORT" != "yes"], [CFLAGS="$OLD_CFLAGS"])
])


AC_DEFUN([CHECK_DEFINE],
[
  AC_MSG_CHECKING([if $2 contains "$1"])
  have_define=no
  AC_RUN_IFELSE([AC_LANG_PROGRAM([
  #include <stdlib.h>
  #include "$2"
  ],[
  #ifdef $1
    exit(0);
  #endif
  exit(1)
  ])], [have_$1="yes"], [have_$1="no"])

  AC_MSG_RESULT([$have_$1])
])




AC_ARG_ENABLE([wall],
[AS_HELP_STRING([--enable-wall], [use -Wall to get maximum compile warnings (default=no)])],
[cf_use_wall=$enableval],
[cf_use_wall=no]
)
AS_IF([test "$cf_use_wall" = "yes"], [
  CHECK_COMPILER_OPT("-Wall")
])


AC_ARG_ENABLE([werror],
[AS_HELP_STRING([--enable-werror], [treat compile warnings as errors (default=no)])],
[cf_use_werror=$enableval],
[cf_use_werror=no]
)
AS_IF([test "$cf_use_werror" = "yes"], [
  CHECK_COMPILER_OPT("-Werror")
])



AC_ARG_ENABLE([pie],
[AS_HELP_STRING([--enable-pie], [build using -fPIE for position independant code, if compiler supports it (default=yes)])],
[cf_use_pie=$enableval],
[cf_use_pie=yes]
)


AS_IF([test "$cf_use_pie" = "yes"], [
  CHECK_COMPILER_OPT("-fPIE")
])



AC_ARG_ENABLE([fortify],
[AS_HELP_STRING([--enable-fortify], [use 'FORTIFY_SOURCE=3' to harden source code (default=no)])],
[cf_use_fortify=$enableval],
[cf_use_fortify=no]
)
AC_ARG_ENABLE([stack-protect],
[AS_HELP_STRING([--enable-stack-protect], [use '-fstack-protector-strong' to harden source code (default=no)])],
[cf_use_stackprotect=$enableval],
[cf_use_stackprotect=no]
)
AC_ARG_ENABLE([harden],
[AS_HELP_STRING([--enable-harden], [use 'FORTIFY_SOURCE=3' and stack-protect to harden source code (default=no)])],
[cf_use_harden=$enableval],
[cf_use_harden=no]
)



AS_IF([test "$cf_use_harden" = "yes"],
[
  cf_use_fortify="yes"
  cf_use_stackprotect="yes"

  CHECK_COMPILER_OPT("-ftrivial-auto-var-init=zero")
  CHECK_COMPILER_OPT("-fstack-clash-protection")
  CHECK_COMPILER_OPT("-fno-strict-overflow")
  CHECK_COMPILER_OPT("-fno-strict-aliasing")
  CHECK_COMPILER_OPT("-fno-delete-null-pointer-checks")
  CHECK_COMPILER_OPT("-fcf-protection=full")
  CHECK_COMPILER_OPT("-mmitigate-rop")
  CHECK_COMPILER_OPT("-fhardened")
  CHECK_COMPILER_OPT("-Wl,-z,relro,-z,now")
  CHECK_COMPILER_OPT("-fPIE")
])





AC_ARG_WITH(libuseful, [  --with-libuseful		use system-wide libUseful (default=auto) ], cf_system_libuseful=$withval )
AC_ARG_ENABLE(ssl, [  --enable-ssl 	only needed if statically linked libuseful is linked against openssl ], cf_use_ssl=$enableval, cf_use_ssl="no" )
AC_ARG_ENABLE(zlib, [  --enable-zlib     only needed if statically linked libuseful is linked against zlib], cf_use_zlib=$enableval, cf_use_zlib="no")



AC_CHECK_FUNCS(prctl)
AC_CHECK_HEADERS([sys/prctl.h])

AC_CHECK_LIB([c],quick_exit,[cf_have_quick_exit=yes],[cf_have_quick_exit=no])
if test "$cf_have_quick_exit" = "yes"
then
AC_DEFINE([HAVE_QUICK_EXIT])
fi

if test "$cf_system_libuseful" != "no"
then
	AC_CHECK_LIB([Useful-5],GetToken,[cf_have_libUseful5=yes],[cf_have_libUseful5=no])
	if test "$cf_have_libUseful5" = "yes"
	then
	AC_CHECK_HEADERS([libUseful-5/libUseful.h])
	else
	AC_CHECK_LIB([Useful-4],GetToken,[cf_have_libUseful4=yes],[cf_have_libUseful4=no])
	AC_CHECK_HEADERS([libUseful-4/libUseful.h])
	fi
fi

if test "$cf_have_libUseful5" = "yes" 
then
  echo "installed libUseful-5 found, using that instead of bundled libUseful."
  echo "if you want to use bundled libUseful, force it with --with-libuseful=no"
  AC_SUBST(LIBUSEFUL_BUNDLED,"")
  AC_DEFINE(HAVE_LIBUSEFUL5)
  dnl Default behavior of AC_CHECK_LIB is to add the library to the LIBS variable
  dnl previous AC_CHECK_LIB don't do this, because an action is supplied for them
  AC_CHECK_LIB([Useful-5],GetToken)

  cf_use_ssl="yes"

elif test "$cf_have_libUseful4" = "yes" 
then
  echo "installed libUseful-4 found, using that instead of bundled libUseful."
  echo "if you want to use bundled libUseful, force it with --with-libuseful=no"
  AC_SUBST(LIBUSEFUL_BUNDLED,"")
  AC_DEFINE(HAVE_LIBUSEFUL4)
  dnl Default behavior of AC_CHECK_LIB is to add the library to the LIBS variable
  dnl previous AC_CHECK_LIB don't do this, because an action is supplied for them
  AC_CHECK_LIB([Useful-4],GetToken)

  cf_use_ssl="yes"
else
  AC_DEFINE(USE_LIBUSEFUL_BUNDLED)
  AC_SUBST(LIBUSEFUL_BUNDLED,"libUseful-bundled/libUseful.a")
  AC_CONFIG_SUBDIRS([libUseful-bundled])
fi

if test "$cf_use_ssl" = "yes"
then
        dnl openssl isn't really needed, but if it exists and we're using a system-wide libUseful, then that will
        dnl likely be linked against it

        AC_CHECK_LIB(crypto,EVP_CIPHER_CTX_init,,)
        cf_have_libcrypto=$ac_cv_lib_crypto_EVP_CIPHER_CTX_init
        
        dnl New versions of openssl seem to lack EVP_CIPHER_CTX_init, so let's look for another function. I've no idea what's going on with openssl, this is why I use libressl instead.
        if test "$cf_have_libcrypto" != "yes"
        then
                AC_CHECK_LIB(crypto,EVP_CIPHER_CTX_new,,)
                cf_have_libcrypto=$ac_cv_lib_crypto_EVP_CIPHER_CTX_new
        fi
        
        
        AC_CHECK_LIB(ssl,SSL_library_init,,)
        cf_have_libssl=$ac_cv_lib_ssl_SSL_library_init
        
        dnl New versions of openssl seem to lack SSL_library_init, so let's look for another function. I've no idea what's going on with openssl, this is why I use libressl instead.
        if test "$cf_have_libssl" != "yes"
        then
                AC_CHECK_LIB(ssl,OPENSSL_init_ssl,,)
                cf_have_libssl=$ac_cv_lib_ssl_OPENSSL_init_ssl
        fi
        
	cf_use_zlib="yes"
fi



if test "$cf_use_zlib" = "yes"
then
	AC_CHECK_LIB(z,deflate,,)
fi

dnl read Makefile.in and write Makefile 
AC_CONFIG_FILES([Makefile])
AC_OUTPUT

