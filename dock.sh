#!/bin/bash

IMAGE_NAME="vrajpat3ll/load_balancer"

# Get local tags
local_tags=($(docker images --format "{{.Tag}}" $IMAGE_NAME))

# Get remote tags from Docker Hub using API and jq
remote_tags=($(curl -s "https://registry.hub.docker.com/v2/repositories/$IMAGE_NAME/tags?page_size=10" | jq -r '.results[].name'))

# Merge tags, remove duplicates
all_tags=($(echo "${local_tags[@]}" "${remote_tags[@]}" | tr ' ' '\n' | sort -u))

if [ ${#all_tags[@]} -eq 0 ]; then
  echo "No tags found locally or in Docker registry for $IMAGE_NAME. Exiting."
  exit 1
fi

echo "Select load balancer mode to tag and push:"
for i in "${!all_tags[@]}"; do
  echo "$((i+1))) ${all_tags[i]}"
done
echo "$(( ${#all_tags[@]} + 1 ))) Custom tag"

read -p "Enter choice [1-$(( ${#all_tags[@]} + 1 ))]: " choice

if [[ $choice -ge 1 && $choice -le ${#all_tags[@]} ]]; then
  TAG=${all_tags[$((choice-1))]}
elif [[ $choice -eq $(( ${#all_tags[@]} + 1 )) ]]; then
  read -p "Enter custom tag: " TAG
else
  echo "Invalid choice. Exiting."
  exit 1
fi

if [ "$(docker images -q $IMAGE_NAME:$TAG)" != "" ]; then
  echo "Docker image $IMAGE_NAME:$TAG already exists locally."
  read -p "Overwrite by rebuilding? [y/N]: " overwrite
  case "$overwrite" in
    y|Y)
      echo "Proceeding to rebuild and overwrite $IMAGE_NAME:$TAG."
      ;;
    *)
      echo "Exiting without rebuilding."
      exit 0
      ;;
  esac
fi

sudo docker login
sudo docker build -t $IMAGE_NAME src/apna-algo -f docker/apna-algo/Dockerfile

sudo docker tag $IMAGE_NAME $IMAGE_NAME:$TAG
echo "Tagged image as $IMAGE_NAME:$TAG"

read -p "Do you want to push $IMAGE_NAME:$TAG to Docker Hub? [y/N]: " push_confirm
case "$push_confirm" in
  y|Y)
    sudo docker push $IMAGE_NAME:$TAG
    echo "Pushed image $IMAGE_NAME:$TAG to Docker Hub"
    ;;
  *)
    echo "Skipping push to Docker Hub."
    exit 0
    ;;
esac

CONFIG_FILE="charts/loadbalancer/values.yaml"

# Update the tag line (handles double or single quotes and spaces)
sed -i 's/tag: *["'"'"'].*["'"'"']/tag: "'"$TAG"'"/' $CONFIG_FILE

echo "Updated $CONFIG_FILE with tag: $TAG"