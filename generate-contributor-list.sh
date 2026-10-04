#!/bin/bash

# Get output file
filePath="$(pwd)/res/contributors.txt"

# Download list of contributors from GitHub and write to file
curl -s "https://api.github.com/repos/dkfans/keeperfx/contributors?per_page=100" | jq -r '.[].login' > $filePath

# Count contributors
contributorCount=$(grep -c '^' "$filePath")

# Show output
cat $filePath
echo ""
echo "Total contributors: $contributorCount"
echo "Output saved to: $filePath"