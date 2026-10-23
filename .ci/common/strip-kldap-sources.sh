#!/bin/sh

if [ -d 3rdparty/kldap ]; then
	(cd 3rdparty/kldap && \
	rm -rf \
		kioworker \
		src/core/autotests \
		src/widgets)
fi
