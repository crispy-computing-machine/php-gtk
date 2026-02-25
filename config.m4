PHP_ARG_ENABLE([gtk],
  [whether to enable php-gtk extension],
  [AS_HELP_STRING([--enable-gtk], [Enable php-gtk extension])],
  [yes])

if test "$PHP_GTK" != "no"; then
  PKG_CHECK_MODULES([GTK3], [gtk+-3.0])

  PHP_EVAL_INCLINE([$GTK3_CFLAGS])
  PHP_EVAL_LIBLINE([$GTK3_LIBS], [GTK_SHARED_LIBADD])

  PHP_NEW_EXTENSION([gtk], [src/gtk.c], [$ext_shared])
  PHP_SUBST([GTK_SHARED_LIBADD])
fi
