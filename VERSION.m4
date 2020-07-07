# VERSION.m4 -- vim: filetype=config
#
# $Id: VERSION.m4,v 28b17d1c4eab 2020/07/07 21:02:51 keith $
#
# Non-copyrightable material; adjust the following definition, to
# assign a common version number to each of the mingwrt and w32api
# MinGW System Library sub-packages.
#
m4_define([__VERSION__],[5.4])

# This is also a convenient place to specify the import library
# associations, and DLLVERSION information for each DLL which may
# be delivered by this package; each specification record takes
# the form of three data fields, comprising:
#
# 1) A tag field, which maps the assignment to a makefile variable
#    substitution reference of the form @MAP_[TAG]_A_DLLVERSION@.
#
# 2) The name of the import library associated with the mapping.
#
# 3) The DLLVERSION for the corresponding DLL; this must be in the
#    libtool current:revision:age notation, and should be updated,
#    as required, in accordance with libtool conventions.
#
m4_define([MINGW_AC_ENUMERATE_DLLVERSION_SPECS],dnl
[MINGW_AC_SET_DLLVERSION([LIBMINGWEX], [libmingwex.dll.a], [4:0:0])dnl
])

# $RCSfile: VERSION.m4,v $: end of file
