#!/bin/sh
# autogen.sh - Generate configure script for chan_dongle

set -e

echo "Generating build system for chan_dongle..."

# Check for required tools
check_tool() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "Error: $1 not found. Please install $2."
        exit 1
    fi
}

check_tool aclocal "automake"
check_tool autoheader "autoconf"
check_tool automake "automake"
check_tool autoconf "autoconf"
check_tool libtoolize "libtool"

# Clean up old generated files
rm -rf autom4te.cache
rm -f aclocal.m4

# Create m4 directory if it doesn't exist
mkdir -p m4

# Run libtoolize
if libtoolize --version >/dev/null 2>&1 || glibtoolize --version >/dev/null 2>&1; then
    echo "Running libtoolize..."
    libtoolize --force --copy 2>/dev/null || glibtoolize --force --copy
fi

# Run aclocal
echo "Running aclocal..."
aclocal -I m4

# Run autoheader
echo "Running autoheader..."
autoheader

# Run automake
echo "Running automake..."
automake --add-missing --copy --foreign

# Run autoconf
echo "Running autoconf..."
autoconf

echo ""
echo "Build system generated successfully!"
echo ""
echo "Next steps:"
echo "  ./configure --help              # Show configuration options"
echo "  ./configure                     # Configure with defaults"
echo "  make                            # Build the module"
echo "  make install                    # Install to Asterisk"
echo ""
