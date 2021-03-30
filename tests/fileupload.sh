#!/bin/bash
# Parameter 1 is ipaddress or fqdn. Parameter 2 is filename
curl --request POST -H "Content-Type: application/octet-stream" --data-binary @$2 http://$1/api/file_upload/www/$1
echo "  "
