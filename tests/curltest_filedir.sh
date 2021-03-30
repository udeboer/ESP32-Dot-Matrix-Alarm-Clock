#!/bin/bash
# Parameter 1 is ip addres or fqdn name
curl --request POST -H "Content-Type: application/json" --data-binary @filedir.json http://$1/api/json/request
