#!/bin/bash

sudo su - ubuntu

echo export GITHUB_ORG='${GITHUB_ORG}' > /home/ubuntu/.env.sh
echo export GITHUB_REPO='${GITHUB_REPO}' >> /home/ubuntu/.env.sh
echo export GITHUB_TOKEN='${GITHUB_TOKEN}' >> /home/ubuntu/.env.sh
echo export RUNNER_LABEL='${RUNNER_LABEL}' >> /home/ubuntu/.env.sh
echo export INSTALL_RUNNER='${INSTALL_RUNNER}' >> /home/ubuntu/.env.sh
