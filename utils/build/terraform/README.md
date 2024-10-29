# Terraform deployment of actions runner on Flexi

The build is a three step process and runs via GitHub Actions CI:

- Step 1: use terraform to provision a VM on Flexi and run a GitHub actions self-hosted runner on it (this steps runs on a normal GitHub-hosted runner)
- Step 2: run the LUMASS build scripts on the self-hosted runner created in Step 1 and upload the AppImage as an artifact on the GitHub build (if we're building a tag, also create a release and upload the App Image to the release)
- Step 3: destroy the resources that were provisioned on Flexi in Step 1

Some configuration is required for the build to work. The following Actions secrets must be set on the GitHub repo:

- `CLOUDS_YAML_CONTENT` should contain the content of the *clouds.yaml* for Flexi, with a single key named *openstack* (the default)
- `FLAVOR_NAME` should be set to the appropriate VM flavor on Flexi, e.g. *balanced1.8cpu16ram*
- `KEY_PAIR` should be set to the name of your key pair on Flexi, e.g. *my-key*
- `STATE_PASSPHRASE` should be set to a passphrase that will be used to encrypt the terraform state file while it is temporarily stored as an artifact of the build
- `TENANT_NAME` should be set to the name of the project on Flexi, e.g. *NeSI-Internal-Sandbox*
- `TF_GITHUB_TOKEN` should be a GitHub fine-grained access token with *Administration* repository permissions (write) on the the LUMASS repo, or a classic personal access token with repo permissions; for more info [see here](https://docs.github.com/en/rest/actions/self-hosted-runners?apiVersion=2022-11-28#create-a-registration-token-for-a-repository)
