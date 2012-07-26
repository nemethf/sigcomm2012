include ../../../Make.vars 

EXTRA_DIST =\
	meta.json

#this is gcc specific: (needed for 4.5.2)
COMMON_FLAGS += -std=c++0x

if PY_ENABLED
AM_CPPFLAGS += $(PYTHON_CPPFLAGS)
endif # PY_ENABLED 

pkglib_LTLIBRARIES =\
	butterfly_app.la

butterfly_app_la_CPPFLAGS = $(AM_CPPFLAGS) -I $(top_srcdir)/src/nox -I $(top_srcdir)/src/nox/coreapps/
butterfly_app_la_SOURCES = butterfly_app.hh butterfly_app.cc ofp_builder.hh ofp_builder.cc
butterfly_app_la_LDFLAGS = -module -export-dynamic

LIBS = ../../../oflib-exp/liboflib_exp.la

NOX_RUNTIMEFILES = meta.json	

all-local: nox-all-local
clean-local: nox-clean-local 
install-exec-hook: nox-install-local
