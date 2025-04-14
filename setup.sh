#!/bin/bash

# --- setup.sh ---
# Clones required external repositories into the 'ext' directory if they don't exist.

# Exit immediately if a command exits with a non-zero status.
set -e

# Define the base directory for external libraries relative to the script location
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
EXT_DIR="${SCRIPT_DIR}/ext"

# Create the ext directory if it doesn't exist
mkdir -p "$EXT_DIR"
cd "$EXT_DIR" || exit 1 # Change into ext dir, exit if it fails

echo "Checking external repositories in '$PWD'..."

# --- Repository Definitions ---
# Format: DIR_NAME GIT_URL [OPTIONAL_BRANCH_OR_TAG]
declare -a REPOS=(
    "eigen https://gitlab.com/libeigen/eigen.git 3.4" # Example: Checkout tag 3.4
    "entt https://github.com/skypjack/entt.git"
    "glfw https://github.com/glfw/glfw.git"
    "glm https://github.com/g-truc/glm.git"
    "imgui https://github.com/ocornut/imgui.git docking" # Example: Checkout docking branch
    "slang https://github.com/shader-slang/slang.git"
    "spdlog https://github.com/gabime/spdlog.git"
    "tinyobjloader https://github.com/tinyobjloader/tinyobjloader.git"
)

# --- Cloning Logic ---
for repo_info in "${REPOS[@]}"; do
    # Split the info string into an array
    read -r -a info <<< "$repo_info"
    DIR_NAME="${info[0]}"
    GIT_URL="${info[1]}"
    CHECKOUT_TARGET="${info[2]}" # Optional: Branch or tag

    if [ -d "$DIR_NAME" ]; then
        echo "- '$DIR_NAME' directory already exists. Skipping clone."
        # Optional: You could add logic here to pull latest changes if needed
        # (cd "$DIR_NAME" && git pull)
    else
        echo "- Cloning '$DIR_NAME' from '$GIT_URL'..."
        git clone --depth 1 --no-tags ${CHECKOUT_TARGET:+--branch ${CHECKOUT_TARGET}} "$GIT_URL" "$DIR_NAME" # Shallow clone by default
        # Explanation of git clone options:
        # --depth 1: Create a shallow clone with only the most recent commit history (faster, smaller download)
        # --no-tags: Don't fetch tags unless explicitly needed by the branch/tag checkout
        # ${CHECKOUT_TARGET:+--branch ${CHECKOUT_TARGET}}: If CHECKOUT_TARGET is set (not empty), add the --branch flag followed by the target. Works for branches and tags.

        # If you need the full history (e.g., for bisecting), remove --depth 1
        # git clone ${CHECKOUT_TARGET:+--branch ${CHECKOUT_TARGET}} "$GIT_URL" "$DIR_NAME"

        echo "  Cloned '$DIR_NAME'."
    fi
done

echo "Setup script finished. All required repositories should be present in '$EXT_DIR'."

# Go back to the original directory (optional)
cd "$SCRIPT_DIR"

exit 0