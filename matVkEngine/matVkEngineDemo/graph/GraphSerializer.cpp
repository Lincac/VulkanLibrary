#include "graph/GraphSerializer.h"

#include "graph/GraphDocument.h"

#include <cctype>
#include <fstream>
#include <sstream>
#include <unordered_map>

namespace mat::demo {

    namespace {

        std::string escapeJson(const std::string& value) {
            std::string out;
            out.reserve(value.size());
            for (char ch : value) {
                switch (ch) {
                    case '\\':
                        out += "\\\\";
                        break;
                    case '"':
                        out += "\\\"";
                        break;
                    case '\n':
                        out += "\\n";
                        break;
                    case '\r':
                        out += "\\r";
                        break;
                    case '\t':
                        out += "\\t";
                        break;
                    default:
                        out += ch;
                        break;
                }
            }
            return out;
        }

        void skipWs(const std::string& text, size_t& pos) {
            while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos])) != 0) {
                ++pos;
            }
        }

        bool expectChar(const std::string& text, size_t& pos, char ch) {
            skipWs(text, pos);
            if (pos >= text.size() || text[pos] != ch) {
                return false;
            }
            ++pos;
            return true;
        }

        bool parseString(const std::string& text, size_t& pos, std::string& out) {
            skipWs(text, pos);
            if (pos >= text.size() || text[pos] != '"') {
                return false;
            }
            ++pos;
            out.clear();
            while (pos < text.size()) {
                const char ch = text[pos++];
                if (ch == '"') {
                    return true;
                }
                if (ch == '\\' && pos < text.size()) {
                    const char esc = text[pos++];
                    switch (esc) {
                        case '\\':
                            out += '\\';
                            break;
                        case '"':
                            out += '"';
                            break;
                        case 'n':
                            out += '\n';
                            break;
                        case 'r':
                            out += '\r';
                            break;
                        case 't':
                            out += '\t';
                            break;
                        default:
                            out += esc;
                            break;
                    }
                    continue;
                }
                out += ch;
            }
            return false;
        }

        bool parseNumber(const std::string& text, size_t& pos, double& out) {
            skipWs(text, pos);
            const size_t start = pos;
            while (pos < text.size() &&
                   (std::isdigit(static_cast<unsigned char>(text[pos])) != 0 || text[pos] == '-' ||
                    text[pos] == '+' || text[pos] == '.' || text[pos] == 'e' || text[pos] == 'E')) {
                ++pos;
            }
            if (start == pos) {
                return false;
            }
            out = std::stod(text.substr(start, pos - start));
            return true;
        }

        bool parseInt(const std::string& text, size_t& pos, int& out) {
            double value = 0.0;
            if (!parseNumber(text, pos, value)) {
                return false;
            }
            out = static_cast<int>(value);
            return true;
        }

        bool parseProps(const std::string& text, size_t& pos, std::unordered_map<std::string, std::string>& props) {
            if (!expectChar(text, pos, '{')) {
                return false;
            }
            skipWs(text, pos);
            if (pos < text.size() && text[pos] == '}') {
                ++pos;
                return true;
            }

            while (pos < text.size()) {
                std::string key;
                if (!parseString(text, pos, key)) {
                    return false;
                }
                if (!expectChar(text, pos, ':')) {
                    return false;
                }
                std::string value;
                if (!parseString(text, pos, value)) {
                    return false;
                }
                props[key] = value;

                skipWs(text, pos);
                if (pos < text.size() && text[pos] == ',') {
                    ++pos;
                    continue;
                }
                break;
            }

            return expectChar(text, pos, '}');
        }

        bool parseNode(const std::string& text, size_t& pos, GraphNode& node) {
            if (!expectChar(text, pos, '{')) {
                return false;
            }

            node.props.clear();
            while (pos < text.size()) {
                std::string key;
                if (!parseString(text, pos, key)) {
                    return false;
                }
                if (!expectChar(text, pos, ':')) {
                    return false;
                }

                if (key == "id") {
                    if (!parseInt(text, pos, node.id)) return false;
                } else if (key == "type") {
                    std::string typeName;
                    if (!parseString(text, pos, typeName)) return false;
                    node.type = nodeTypeFromName(typeName);
                } else if (key == "x") {
                    double x = 0.0;
                    if (!parseNumber(text, pos, x)) return false;
                    node.posX = static_cast<float>(x);
                } else if (key == "y") {
                    double y = 0.0;
                    if (!parseNumber(text, pos, y)) return false;
                    node.posY = static_cast<float>(y);
                } else if (key == "props") {
                    if (!parseProps(text, pos, node.props)) return false;
                } else {
                    return false;
                }

                skipWs(text, pos);
                if (pos < text.size() && text[pos] == ',') {
                    ++pos;
                    continue;
                }
                break;
            }

            applyDefaultPins(node);
            return expectChar(text, pos, '}');
        }

        bool parseLink(const std::string& text, size_t& pos, GraphLink& link) {
            if (!expectChar(text, pos, '{')) {
                return false;
            }

            while (pos < text.size()) {
                std::string key;
                if (!parseString(text, pos, key)) {
                    return false;
                }
                if (!expectChar(text, pos, ':')) {
                    return false;
                }

                if (key == "id") {
                    if (!parseInt(text, pos, link.id)) return false;
                } else if (key == "from") {
                    if (!parseInt(text, pos, link.startAttr)) return false;
                } else if (key == "to") {
                    if (!parseInt(text, pos, link.endAttr)) return false;
                } else {
                    return false;
                }

                skipWs(text, pos);
                if (pos < text.size() && text[pos] == ',') {
                    ++pos;
                    continue;
                }
                break;
            }

            return expectChar(text, pos, '}');
        }

        bool parseArrayNodes(const std::string& text, size_t& pos, std::vector<GraphNode>& nodes) {
            if (!expectChar(text, pos, '[')) {
                return false;
            }
            skipWs(text, pos);
            if (pos < text.size() && text[pos] == ']') {
                ++pos;
                return true;
            }

            while (pos < text.size()) {
                GraphNode node{};
                if (!parseNode(text, pos, node)) {
                    return false;
                }
                nodes.push_back(node);

                skipWs(text, pos);
                if (pos < text.size() && text[pos] == ',') {
                    ++pos;
                    continue;
                }
                break;
            }

            return expectChar(text, pos, ']');
        }

        bool parseArrayLinks(const std::string& text, size_t& pos, std::vector<GraphLink>& links) {
            if (!expectChar(text, pos, '[')) {
                return false;
            }
            skipWs(text, pos);
            if (pos < text.size() && text[pos] == ']') {
                ++pos;
                return true;
            }

            while (pos < text.size()) {
                GraphLink link{};
                if (!parseLink(text, pos, link)) {
                    return false;
                }
                links.push_back(link);

                skipWs(text, pos);
                if (pos < text.size() && text[pos] == ',') {
                    ++pos;
                    continue;
                }
                break;
            }

            return expectChar(text, pos, ']');
        }

    }  // namespace

    bool GraphSerializer::save(const GraphDocument& doc, const std::string& path, std::string& error) {
        std::ofstream out(path, std::ios::binary);
        if (!out.is_open()) {
            error = "Failed to open file for writing: " + path;
            return false;
        }

        out << "{\n";
        out << "  \"version\": 1,\n";
        out << "  \"nextNodeId\": " << doc.nextNodeId() << ",\n";
        out << "  \"nextLinkId\": " << doc.nextLinkId() << ",\n";
        out << "  \"nodes\": [\n";

        for (size_t i = 0; i < doc.nodes().size(); ++i) {
            const GraphNode& node = doc.nodes()[i];
            out << "    {\n";
            out << "      \"id\": " << node.id << ",\n";
            out << "      \"type\": \"" << nodeTypeName(node.type) << "\",\n";
            out << "      \"x\": " << node.posX << ",\n";
            out << "      \"y\": " << node.posY << ",\n";
            out << "      \"props\": {\n";

            size_t propIndex = 0;
            for (const auto& [key, value] : node.props) {
                out << "        \"" << escapeJson(key) << "\": \"" << escapeJson(value) << "\"";
                if (++propIndex < node.props.size()) {
                    out << ",";
                }
                out << "\n";
            }

            out << "      }\n";
            out << "    }";
            if (i + 1 < doc.nodes().size()) {
                out << ",";
            }
            out << "\n";
        }

        out << "  ],\n";
        out << "  \"links\": [\n";

        for (size_t i = 0; i < doc.links().size(); ++i) {
            const GraphLink& link = doc.links()[i];
            out << "    { \"id\": " << link.id << ", \"from\": " << link.startAttr << ", \"to\": " << link.endAttr
                << " }";
            if (i + 1 < doc.links().size()) {
                out << ",";
            }
            out << "\n";
        }

        out << "  ]\n";
        out << "}\n";
        return true;
    }

    bool GraphSerializer::load(GraphDocument& doc, const std::string& path, std::string& error) {
        std::ifstream in(path, std::ios::binary);
        if (!in.is_open()) {
            error = "Failed to open file for reading: " + path;
            return false;
        }

        std::ostringstream buffer;
        buffer << in.rdbuf();
        const std::string text = buffer.str();

        size_t pos = 0;
        if (!expectChar(text, pos, '{')) {
            error = "Invalid graph file";
            return false;
        }

        std::vector<GraphNode> nodes;
        std::vector<GraphLink> links;
        int nextNodeId = 1;
        int nextLinkId = 1;

        while (pos < text.size()) {
            skipWs(text, pos);
            if (pos < text.size() && text[pos] == '}') {
                ++pos;
                break;
            }

            std::string key;
            if (!parseString(text, pos, key)) {
                break;
            }
            if (!expectChar(text, pos, ':')) {
                error = "Invalid graph file";
                return false;
            }

            if (key == "version") {
                double version = 0.0;
                if (!parseNumber(text, pos, version)) {
                    error = "Invalid version";
                    return false;
                }
            } else if (key == "nextNodeId") {
                if (!parseInt(text, pos, nextNodeId)) {
                    error = "Invalid nextNodeId";
                    return false;
                }
            } else if (key == "nextLinkId") {
                if (!parseInt(text, pos, nextLinkId)) {
                    error = "Invalid nextLinkId";
                    return false;
                }
            } else if (key == "nodes") {
                if (!parseArrayNodes(text, pos, nodes)) {
                    error = "Invalid nodes array";
                    return false;
                }
            } else if (key == "links") {
                if (!parseArrayLinks(text, pos, links)) {
                    error = "Invalid links array";
                    return false;
                }
            } else {
                error = "Unknown key: " + key;
                return false;
            }

            skipWs(text, pos);
            if (pos < text.size() && text[pos] == ',') {
                ++pos;
            }
        }

        GraphDocument loaded{};
        for (GraphNode& node : nodes) {
            loaded._nodes.push_back(std::move(node));
        }
        for (GraphLink& link : links) {
            loaded._links.push_back(std::move(link));
        }
        loaded._nextNodeId = nextNodeId;
        loaded._nextLinkId = nextLinkId;

        doc = std::move(loaded);
        return true;
    }

}  // namespace mat::demo
