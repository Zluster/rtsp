#!/bin/bash
set -e

echo "🚀 Starting API documentation generation..."

# 1. 检查依赖
if ! command -v doxygen &> /dev/null; then
    echo "❌ Error: doxygen not found. Install with: sudo apt install doxygen graphviz"
    exit 1
fi

# 2. 清理旧文档
echo "🧹 Cleaning old docs..."
rm -rf docs/doxygen/output

# 3. 生成新文档
echo "📄 Generating documentation with Doxygen..."
doxygen docs/doxygen/config/Doxyfile

# 4. 验证输出
if [ ! -f "docs/doxygen/output/html/index.html" ]; then
    echo "❌ Error: docs/doxygen/output/html/index.html not generated!"
    exit 1
fi

echo "✅ Documentation generated successfully!"
echo "📁 Output: $(pwd)/docs/doxygen/output/html/index.html"
echo ""
echo "🔗 Open in browser:"
echo "   file://$(pwd)/docs/doxygen/output/html/index.html"
echo ""
echo "🌐 Serve with Python for online access:"
echo "   cd docs/doxygen/output/html && python3 -m http.server 8000"
echo ""
echo "🐳 Or serve with Docker:"
echo "   docker run -p 8000:80 -v $(pwd)/docs/doxygen/output/html:/usr/local/apache2/htdocs/ httpd:2.4"
