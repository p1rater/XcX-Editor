## Installation

Download the latest binary and verify its integrity:

```bash
wget https://github.com/p1rater/XcX-Editor/releases/download/v1.0/xcx
wget https://github.com/p1rater/XcX-Editor/releases/download/v1.0/xcx.asc
wget https://github.com/p1rater/XcX-Editor/releases/download/v1.0/xcx-pubkey.asc

# Import the public key
gpg --import xcx-pubkey.asc

# Verify the signature
gpg --verify xcx.asc xcx

# Check SHA256 checksum
sha256sum -c xcx.sha256

# Make executable and run
chmod +x xcx
./xcx

# Usage
./xcx – open current directory
./xcx file.txt – open specific file
./xcx /path/to/dir – open directory as file tree

