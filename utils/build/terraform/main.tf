# define providers
terraform {
required_version = ">= 0.14.0"
  required_providers {
    openstack = {
      source  = "terraform-provider-openstack/openstack"
      version = "~> 1.53.0"
    }
  }

  backend "s3" {
    bucket = "chris-lumass-terraform"
    key    = "state/terraform.tfstate"
    region = "us-east-1"
    use_path_style = "true"
    skip_credentials_validation = "true"
    endpoints = {
      s3 = "https://object.akl-1.cloud.nesi.org.nz/"
    }
    skip_requesting_account_id = "true"
    skip_s3_checksum = "true"
  }
}

# Configure the OpenStack Provider
provider "openstack" {
  cloud = var.openstack_cloud
}

# define some data sources
data "openstack_networking_network_v2" "net" {
    name = var.tenant_name
}
data "openstack_images_image_v2" "image" {
    name = var.image_name
}
data "openstack_compute_flavor_v2" "flavor" {
    name = var.flavor_name
}

# security group
resource "openstack_networking_secgroup_v2" "secgroup" {
  name        = "runner_secgroup"
  description = "Security group for LUMASS runner"
  delete_default_rules = true
}
# allow egress
resource "openstack_networking_secgroup_rule_v2" "secgroup_rule_v4" {
  direction         = "egress"
  ethertype         = "IPv4"
  security_group_id = openstack_networking_secgroup_v2.secgroup.id
}
resource "openstack_networking_secgroup_rule_v2" "secgroup_rule_v6" {
  direction         = "egress"
  ethertype         = "IPv6"
  security_group_id = openstack_networking_secgroup_v2.secgroup.id
}
# optionally allow in SSH
resource "openstack_networking_secgroup_rule_v2" "secgroup_rule_ssh" {
  count = var.enable_debugging ? 1 : 0
  direction         = "ingress"
  ethertype         = "IPv4"
  protocol          = "tcp"
  port_range_min    = 22
  port_range_max    = 22
  remote_ip_prefix  = "0.0.0.0/0"
  security_group_id = openstack_networking_secgroup_v2.secgroup.id
}

# port
resource "openstack_networking_port_v2" "port" {
    name = "runner_port"
    network_id = data.openstack_networking_network_v2.net.id
    security_group_ids = [openstack_networking_secgroup_v2.secgroup.id]
}

# optional floating ip
resource "openstack_networking_floatingip_v2" "floatingip" {
  count = var.enable_debugging ? 1 : 0
  pool = "external"
}
resource "openstack_networking_floatingip_associate_v2" "fip" {
  count = var.enable_debugging ? 1 : 0
  floating_ip = openstack_networking_floatingip_v2.floatingip[0].address
  port_id     = openstack_networking_port_v2.port.id
}

# cloudinit to bootstrap a runner host
data "cloudinit_config" "runner_config" {
  # cloud-init.cfg
  part {
    filename     = "cloud-init.cfg"
    content_type = "text/cloud-config"
    content = file("${path.module}/setup-scripts/cloud-init.cfg")
  }

  # generate-env.sh
  part {
    content_type = "text/x-shellscript"
    content = templatefile("${path.module}/setup-scripts/generate-env.sh.tpl", {
      GITHUB_ORG = var.github_org
      GITHUB_REPO = var.github_repo
      GITHUB_TOKEN = var.github_token
    })
  }

  # install-runner.sh
  part {
    content_type = "text/x-shellscript"
    content      = file("${path.module}/setup-scripts/install-runner.sh")
  }
}

# Create the actions runner compute instance
resource "openstack_compute_instance_v2" "runner_instance" {
  name            = var.instance_name
  flavor_id       = data.openstack_compute_flavor_v2.flavor.id
  key_pair        = var.key_pair

  user_data = data.cloudinit_config.runner_config.rendered

  block_device {
    uuid = data.openstack_images_image_v2.image.id
    source_type = "image"
    destination_type = "volume"
    boot_index = 0
    volume_size = var.runner_volume_size
    delete_on_termination = true
  }

  network {
    port = openstack_networking_port_v2.port.id
  }
}
