# ogg.m4
# Configure paths for libogg
# Jack Moffitt <jack@icecast.org> 10-21-2000
# Shamelessly stolen from Owen Taylor and Manish Singh

dnl AM_PATH_OGG([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl Test for libogg, and define OGG_CFLAGS and OGG_LIBS
dnl
AC_DEFUN(AM_PATH_OGG,
[dnl 
dnl Get the cflags and libraries
dnl
AC_ARG_WITH(ogg-prefix,[  --with-ogg-prefix=PFX   Prefix where libogg is installed (optional)], ogg_prefix="$withval", ogg_prefix="")
AC_ARG_ENABLE(oggtest, [  --disable-oggtest       Do not try to compile and run a test Ogg program],, enable_oggtest=yes)

  if test "x$ogg_prefix" != "xNONE" ; then
    ogg_args="$ogg_args --prefix=$ogg_prefix"
    OGG_CFLAGS="-I$ogg_prefix/include"
    OGG_LIBS="-L$ogg_prefix/lib"
  elif test "$prefix" != ""; then
    ogg_args="$ogg_args --prefix=$prefix"
    OGG_CFLAGS="-I$prefix/include"
    OGG_LIBS="-L$prefix/lib"
  fi

  OGG_LIBS="$OGG_LIBS -logg"

  AC_MSG_CHECKING(for Ogg)
  no_ogg=""


  if test "x$enable_oggtest" = "xyes" ; then
    ac_save_CFLAGS="$CFLAGS"
    ac_save_LIBS="$LIBS"
    CFLAGS="$CFLAGS $OGG_CFLAGS"
    LIBS="$LIBS $OGG_LIBS"
dnl
dnl Now check if the installed Ogg is sufficiently new.
dnl
      rm -f conf.oggtest
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ogg/ogg.h>

int main ()
{
  system("touch conf.oggtest");
  return 0;
}

],, no_ogg=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
  fi

  if test "x$no_ogg" = "x" ; then
     AC_MSG_RESULT(yes)
     ifelse([$1], , :, [$1])     
  else
     AC_MSG_RESULT(no)
     if test -f conf.oggtest ; then
       :
     else
       echo "*** Could not run Ogg test program, checking why..."
       CFLAGS="$CFLAGS $OGG_CFLAGS"
       LIBS="$LIBS $OGG_LIBS"
       AC_TRY_LINK([
#include <stdio.h>
#include <ogg/ogg.h>
],     [ return 0; ],
       [ echo "*** The test program compiled, but did not run. This usually means"
       echo "*** that the run-time linker is not finding Ogg or finding the wrong"
       echo "*** version of Ogg. If it is not finding Ogg, you'll need to set your"
       echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
       echo "*** to the installed location  Also, make sure you have run ldconfig if that"
       echo "*** is required on your system"
       echo "***"
       echo "*** If you have an old version installed, it is best to remove it, although"
       echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
       [ echo "*** The test program failed to compile or link. See the file config.log for the"
       echo "*** exact error that occured. This usually means Ogg was incorrectly installed"
       echo "*** or that you have moved Ogg since it was installed. In the latter case, you"
       echo "*** may want to edit the ogg-config script: $OGG_CONFIG" ])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
     OGG_CFLAGS=""
     OGG_LIBS=""
     ifelse([$2], , :, [$2])
  fi
  AC_SUBST(OGG_CFLAGS)
  AC_SUBST(OGG_LIBS)
  rm -f conf.oggtest
])

# vorbis.m4
# Configure paths for libvorbis
# Jack Moffitt <jack@icecast.org> 10-21-2000
# Shamelessly stolen from Owen Taylor and Manish Singh

dnl AM_PATH_VORBIS([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl Test for libvorbis, and define VORBIS_CFLAGS and VORBIS_LIBS
dnl
AC_DEFUN(AM_PATH_VORBIS,
[dnl 
dnl Get the cflags and libraries
dnl
AC_ARG_WITH(vorbis-prefix,[  --with-vorbis-prefix=PFX   Prefix where libvorbis is installed (optional)], vorbis_prefix="$withval", vorbis_prefix="")
AC_ARG_ENABLE(vorbistest, [  --disable-vorbistest       Do not try to compile and run a test Vorbis program],, enable_vorbistest=yes)

  if test "x$vorbis_prefix" != "xNONE" ; then
    vorbis_args="$vorbis_args --prefix=$vorbis_prefix"
    VORBIS_CFLAGS="-I$vorbis_prefix/include"
    VORBIS_LIBDIR="-L$vorbis_prefix/lib"
  elif test "$prefix" != ""; then
    vorbis_args="$vorbis_args --prefix=$prefix"
    VORBIS_CFLAGS="-I$prefix/include"
    VORBIS_LIBDIR="-L$prefix/lib"
  fi

  VORBIS_LIBS="$VORBIS_LIBDIR -lvorbis -lm"
  VORBISFILE_LIBS="-lvorbisfile"
  VORBISENC_LIBS="-lvorbisenc"

  AC_MSG_CHECKING(for Vorbis)
  no_vorbis=""


  if test "x$enable_vorbistest" = "xyes" ; then
    ac_save_CFLAGS="$CFLAGS"
    ac_save_LIBS="$LIBS"
    CFLAGS="$CFLAGS $VORBIS_CFLAGS"
    LIBS="$LIBS $VORBIS_LIBS $OGG_LIBS"
dnl
dnl Now check if the installed Vorbis is sufficiently new.
dnl
      rm -f conf.vorbistest
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vorbis/codec.h>

int main ()
{
  system("touch conf.vorbistest");
  return 0;
}

],, no_vorbis=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
  fi

  if test "x$no_vorbis" = "x" ; then
     AC_MSG_RESULT(yes)
     ifelse([$1], , :, [$1])     
  else
     AC_MSG_RESULT(no)
     if test -f conf.vorbistest ; then
       :
     else
       echo "*** Could not run Vorbis test program, checking why..."
       CFLAGS="$CFLAGS $VORBIS_CFLAGS"
       LIBS="$LIBS $VORBIS_LIBS $OGG_LIBS"
       AC_TRY_LINK([
#include <stdio.h>
#include <vorbis/codec.h>
],     [ return 0; ],
       [ echo "*** The test program compiled, but did not run. This usually means"
       echo "*** that the run-time linker is not finding Vorbis or finding the wrong"
       echo "*** version of Vorbis. If it is not finding Vorbis, you'll need to set your"
       echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
       echo "*** to the installed location  Also, make sure you have run ldconfig if that"
       echo "*** is required on your system"
       echo "***"
       echo "*** If you have an old version installed, it is best to remove it, although"
       echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
       [ echo "*** The test program failed to compile or link. See the file config.log for the"
       echo "*** exact error that occured. This usually means Vorbis was incorrectly installed"
       echo "*** or that you have moved Vorbis since it was installed." ])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
     VORBIS_CFLAGS=""
     VORBIS_LIBS=""
     VORBISFILE_LIBS=""
     VORBISENC_LIBS=""
     ifelse([$2], , :, [$2])
  fi
  AC_SUBST(VORBIS_CFLAGS)
  AC_SUBST(VORBIS_LIBS)
  AC_SUBST(VORBISFILE_LIBS)
  AC_SUBST(VORBISENC_LIBS)
  rm -f conf.vorbistest
])

# ao.m4
# Configure paths for libao
# Jack Moffitt <jack@icecast.org> 10-21-2000
# Shamelessly stolen from Owen Taylor and Manish Singh

dnl AM_PATH_AO([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl Test for libao, and define AO_CFLAGS and AO_LIBS
dnl
AC_DEFUN(AM_PATH_AO,
[dnl 
dnl Get the cflags and libraries
dnl
AC_ARG_WITH(ao-prefix,[  --with-ao-prefix=PFX   Prefix where libao is installed (optional)], ao_prefix="$withval", ao_prefix="")
AC_ARG_ENABLE(aotest, [  --disable-aotest       Do not try to compile and run a test ao program],, enable_aotest=yes)

  if test "x$ao_prefix" != "xNONE" ; then
    ao_args="$ao_args --prefix=$ao_prefix"
    AO_CFLAGS="-I$ao_prefix/include"
    AO_LIBS="-L$ao_prefix/lib"
  elif test "$prefix" != ""; then
    ao_args="$ao_args --prefix=$prefix"
    AO_CFLAGS="-I$prefix/include"
    AO_LIBS="-L$prefix/lib"
  fi

  # see where dl* and friends live
  AC_CHECK_FUNCS(dlopen, [AO_DL_LIBS=""], [
    AC_CHECK_LIB(dl, dlopen, [AO_DL_LIBS="-ldl"], [
      AC_MSG_ERROR([could not find dlopen() needed by libao sound drivers
      your system may not be supported.])
    ])
  ])

  AO_LIBS="$AO_LIBS -lao $AO_DL_LIBS"

  AC_MSG_CHECKING(for ao)
  no_ao=""


  if test "x$enable_aotest" = "xyes" ; then
    ac_save_CFLAGS="$CFLAGS"
    ac_save_LIBS="$LIBS"
    CFLAGS="$CFLAGS $AO_CFLAGS"
    LIBS="$LIBS $AO_LIBS"
dnl
dnl Now check if the installed ao is sufficiently new.
dnl
      rm -f conf.aotest
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ao/ao.h>

int main ()
{
  system("touch conf.aotest");
  return 0;
}

],, no_ao=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
  fi

  if test "x$no_ao" = "x" ; then
     AC_MSG_RESULT(yes)
     ifelse([$1], , :, [$1])     
  else
     AC_MSG_RESULT(no)
     if test -f conf.aotest ; then
       :
     else
       echo "*** Could not run ao test program, checking why..."
       CFLAGS="$CFLAGS $AO_CFLAGS"
       LIBS="$LIBS $AO_LIBS"
       AC_TRY_LINK([
#include <stdio.h>
#include <ao/ao.h>
],     [ return 0; ],
       [ echo "*** The test program compiled, but did not run. This usually means"
       echo "*** that the run-time linker is not finding ao or finding the wrong"
       echo "*** version of ao. If it is not finding ao, you'll need to set your"
       echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
       echo "*** to the installed location  Also, make sure you have run ldconfig if that"
       echo "*** is required on your system"
       echo "***"
       echo "*** If you have an old version installed, it is best to remove it, although"
       echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
       [ echo "*** The test program failed to compile or link. See the file config.log for the"
       echo "*** exact error that occured. This usually means ao was incorrectly installed"
       echo "*** or that you have moved ao since it was installed." ])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
     AO_CFLAGS=""
     AO_LIBS=""
     ifelse([$2], , :, [$2])
  fi
  AC_SUBST(AO_CFLAGS)
  AC_SUBST(AO_LIBS)
  rm -f conf.aotest
])
