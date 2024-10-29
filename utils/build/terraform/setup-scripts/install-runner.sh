#!/bin/bash

set -e -o pipefail

LOG_FILE=/home/ubuntu/install-runner.log
RUNNER_VERSION="2.320.0"

cat <<EOF > /home/ubuntu/install-runner.sh
    #!/bin/bash

    set -e -o pipefail

    cd /home/ubuntu
    source /home/ubuntu/.env.sh

    mkdir actions-runner && cd actions-runner
    echo "Download actions runner"
    curl -o actions-runner-linux-x64-${RUNNER_VERSION}.tar.gz -L https://github.com/actions/runner/releases/download/v${RUNNER_VERSION}/actions-runner-linux-x64-${RUNNER_VERSION}.tar.gz
    echo "93ac1b7ce743ee85b5d386f5c1787385ef07b3d7c728ff66ce0d3813d5f46900  actions-runner-linux-x64-${RUNNER_VERSION}.tar.gz" | shasum -a 256 -c
    tar xzf ./actions-runner-linux-x64-${RUNNER_VERSION}.tar.gz

    ACTIONS_URL="https://api.github.com/repos/\${GITHUB_REPO}/actions/runners/registration-token"
    echo "Requesting registration token at: \${ACTIONS_URL}"
    PAYLOAD=\$(curl -sX POST -H "Authorization: token \${GITHUB_TOKEN}" \${ACTIONS_URL})
    export RUNNER_TOKEN=\$(echo \$PAYLOAD | jq .token --raw-output)
    if [ -z "\$RUNNER_TOKEN" ] || [ "\$RUNNER_TOKEN" == "null" ]; then
        echo "Failed to get RUNNER_TOKEN, check GITHUB_TOKEN is valid, details follow..."
        echo \$PAYLOAD
        exit 1
    fi

    WORKDIR=/home/ubuntu/runner-workdir
    rm -rf "\${WORKDIR}"
    mkdir -p "\${WORKDIR}"

    # configuring the runner
    echo "Configuring the runner..."
    ./config.sh \
        --name "\${RUNNER_LABEL}" \
        --token "\${RUNNER_TOKEN}" \
        --url "https://github.com/\${GITHUB_REPO}" \
        --work "\${WORKDIR}" \
        --unattended \
        --ephemeral \
        --no-default-labels \
        --labels "\${RUNNER_LABEL}"

    # start the runner
    echo "Starting the runner..."
    sudo ./svc.sh install
    sudo ./svc.sh start
EOF

source /home/ubuntu/.env.sh

if [ "$INSTALL_RUNNER" = "1" ]; then
    chmod +x /home/ubuntu/install-runner.sh
    su - ubuntu -c /home/ubuntu/install-runner.sh >& $LOG_FILE
fi
