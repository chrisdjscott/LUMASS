variable "openstack_cloud" {
  description = "OpenStack cloud provider key from clouds.yaml"
  default = "openstack"
}

variable "instance_name" {
  description = "Name for compute instance"
  default = "lumass-actions-runner"
}

variable "tenant_name" {
  description = "FlexiHPC project Name"
}

variable "key_pair" {
  description = "FlexiHPC Key Pair name"
}

variable "flavor_name" {
  description = "FlexiHPC flavor name"
  default     = "balanced1.8cpu16ram"
}

variable "image_name" {
  description = "FlexiHPC image name"
  default     = "NeSI-FlexiHPC-Ubuntu-Jammy_22.04"
}

variable "runner_volume_size" {
  description = "VM disk size (GB), defaults to 30"
  default     = "30"
}

variable "vm_user" {
  description = "FlexiHPC VM user"
  default = "ubuntu"
}

variable "github_org" {
  description = "GitHub organisation of the repo to add the self-hosted runner to"
}

variable "github_repo" {
  description = "Name of the GitHub repo to add the self-hosted runner to"
}

variable "github_token" {
  description = "GitHub personal access token"
}

variable "enable_debugging" {
  description = "Enable debugging the VM (add floating IP and allow SSH in)"
  type = bool
  default = false
}
