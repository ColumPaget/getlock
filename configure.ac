AC_INIT
AC_CONFIG_SRCDIR([main.c])


AC_PROG_CC 
AC_LANG([C]) 
AC_PROG_MAKE_SET 
AC_PROG_INSTALL 
AC_PROG_EGREP
 
AC_SYS_LARGEFILE

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
  AC_DEFINE(HAVE_LIBUSEFUL5)
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

