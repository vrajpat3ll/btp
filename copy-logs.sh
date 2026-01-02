#!/bin/bash

sudo kubectl cp loadbalancer/lb-0:lb/logs data/logs

sudo chown btp:btp -R data/logs