# Terraform deployment of actions runner on Flexi

Create a fine-grained access tokenb with "Administration" repository permissions (write) on the repo (https://docs.github.com/en/rest/actions/self-hosted-runners?apiVersion=2022-11-28#create-a-registration-token-for-a-repository)

## TODO

- only one job at a time
- what to use for backend?
- option to allow ssh in and create floatingip
- wait few mins for cloud init to complete (how to determine this automatically)
