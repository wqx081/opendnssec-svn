#!/usr/bin/env bash

## Test basic Input DNS Adapter
## See if zone gets transferred and signed. See if we can transfer the
## 

## It requires setting up a zone in OpenDNSSEC with Input DNS Adapter,
## non-default zonelist.xml, non-default conf.xml, additional addns.xml.
## It requires setting up a primary name server (ldns-testns).
## It requires a checker tool like wdiff or ldns-verify-zone to review
## the result (possibly with an known good file).

if [ -n "$HAVE_MYSQL" ]; then
	ods_setup_conf conf.xml conf-mysql.xml
fi &&

ods_reset_env &&

## Start master name server
ods_ldns_testns 15353 ods.datafile &&

## Start OpenDNSSEC
log_this_timeout ods-control-start 60 ods-control start &&
syslog_waitfor 60 'ods-enforcerd: .*Sleeping for' &&
syslog_waitfor 60 'ods-signerd: .*\[engine\] signer started' &&

## Wait for signed zone file
syslog_waitfor 60 'ods-signerd: .*\[STATS\] ods' &&

## Check signed zone file [when we decide on auditor tool]
test -f "$INSTALL_ROOT/var/opendnssec/signed/ods" &&

## See if REFRESH is being done
ods-signer verbosity 5 &&
syslog_waitfor 35 'ods-signerd: .*\[xfrd\] zone ods make request round 0 master' &&
syslog_waitfor 5 'ods-signerd: .*\[xfrd\] zone ods got update indicating current serial' &&
syslog_waitfor 5 'ods-signerd: .*\[xfrd\] zone ods sets timer timeout refresh 30' &&

## Stop
log_this_timeout ods-control-stop 60 ods-control stop &&
syslog_waitfor 60 'ods-enforcerd: .*all done' &&
syslog_waitfor 60 'ods-signerd: .*\[engine\] signer shutdown' &&
ods_ldns_testns_kill &&
return 0

## Test failed. Kill stuff
ods_ldns_testns_kill
ods_kill
return 1