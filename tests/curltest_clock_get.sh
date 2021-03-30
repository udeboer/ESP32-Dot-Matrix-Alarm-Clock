#!/bin/bash
# Parameter 1 is ip address or fqdn
curl --request POST -H "Content-Type: application/json" --data-binary @clockget.json http://$1/api/json/request
