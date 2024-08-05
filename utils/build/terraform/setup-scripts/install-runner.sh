#!/bin/bash

set -e -o pipefail

LOG_FILE=/home/ubuntu/install-runner.log

cat <<EOF > /home/ubuntu/install-runner.sh
    #!/bin/bash

    set -e -o pipefail

    cd /home/ubuntu
    source /home/ubuntu/.env.sh

    mkdir actions-runner && cd actions-runner
    echo "Download actions runner"
    curl -o actions-runner-linux-x64-2.317.0.tar.gz -L https://github.com/actions/runner/releases/download/v2.317.0/actions-runner-linux-x64-2.317.0.tar.gz
    echo "9e883d210df8c6028aff475475a457d380353f9d01877d51cc01a17b2a91161d  actions-runner-linux-x64-2.317.0.tar.gz" | shasum -a 256 -c
    tar xzf ./actions-runner-linux-x64-2.317.0.tar.gz

    ACTIONS_URL="https://api.github.com/repos/\${GITHUB_ORG}/\${GITHUB_REPO}/actions/runners/registration-token"
    echo "Requesting registration token at: \${ACTIONS_URL}"

    PAYLOAD=\$(curl -sX POST -H "Authorization: token \${GITHUB_TOKEN}" \${ACTIONS_URL})
    export RUNNER_TOKEN=\$(echo \$PAYLOAD | jq .token --raw-output)
    echo "Got token"

    WORKDIR=/home/ubuntu/runner-workdir
    rm -rf "\${WORKDIR}"
    mkdir -p "\${WORKDIR}"

    # configuring the runner
    ./config.sh \
        --name "lumass-flexi-runner" \
        --token "\${RUNNER_TOKEN}" \
        --url "https://github.com/\${GITHUB_ORG}/\${GITHUB_REPO}" \
        --work "\${WORKDIR}" \
        --unattended \
        --ephemeral \
        --labels "\${RUNNER_LABEL}" \
        --replace

    # start the runner
    sudo ./svc.sh install
    sudo ./svc.sh start
EOF

chmod +x /home/ubuntu/install-runner.sh
su - ubuntu -c /home/ubuntu/install-runner.sh >& $LOG_FILE
