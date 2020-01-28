
lib_LTLIBRARIES = libaio-oracle.la

libaio_oracle_la_LDFLAGS =					\
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)	\
	@AIO_LIBS@

libaio_oracleincludedir = $(includedir)/libaio-oracle/
libaio_oracleinclude_HEADERS =	\
	libaio-oracle.h

libaio_oracle_la_SOURCES = libaio-oracle.c

noinst_PROGRAMS = lio_test oratest poll_test

lio_test_SOURCES = lio_test.c

lio_test_LDADD =		\
	libaio-oracle.la	\
	@AIO_LIBS@

poll_test_SOURCES = poll_test.c

poll_test_LDADD =		\
	libaio-oracle.la	\
	@AIO_LIBS@

oratest_SOURCES = oratest.c

oratest_LDADD =		\
	libaio-oracle.la	\
	@AIO_LIBS@

man_MANS = 

