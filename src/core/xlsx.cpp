#include "xlsx.h"
#include "zip.h"

#include <sstream>

namespace core {

namespace {

std::string xmlEscape(const std::string& s)
{
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
        case '&': out += "&amp;"; break;
        case '<': out += "&lt;"; break;
        case '>': out += "&gt;"; break;
        case '"': out += "&quot;"; break;
        default: out += c; break;
        }
    }
    return out;
}

std::string sheetXml(const std::vector<std::vector<std::string>>& rows)
{
    std::ostringstream ss;
    ss << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
       << "<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">"
       << "<sheetData>\n";
    for (size_t r = 0; r < rows.size(); ++r) {
        ss << "<row r=\"" << (r + 1) << "\">";
        for (size_t c = 0; c < rows[r].size(); ++c) {
            const std::string ref = std::string(1, char('A' + (c % 26))) + std::to_string(r + 1);
            ss << "<c r=\"" << ref << "\" t=\"inlineStr\"><is><t>"
               << xmlEscape(rows[r][c]) << "</t></is></c>";
        }
        ss << "</row>\n";
    }
    ss << "</sheetData></worksheet>";
    return ss.str();
}

} // namespace

std::string xlsxWrite(const std::vector<std::vector<std::string>>& rows)
{
    const std::string sheet = sheetXml(rows);
    const std::string contentTypes =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
        "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
        "<Default Extension=\"xml\" ContentType=\"application/xml\"/>"
        "<Override PartName=\"/xl/workbook.xml\" "
        "ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml\"/>"
        "<Override PartName=\"/xl/worksheets/sheet1.xml\" "
        "ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml\"/>"
        "</Types>";
    const std::string rels =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
        "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" "
        "Target=\"xl/workbook.xml\"/>"
        "</Relationships>";
    const std::string workbook =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<workbook xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\" "
        "xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\">"
        "<sheets><sheet name=\"Timesheet\" sheetId=\"1\" r:id=\"rId1\"/></sheets></workbook>";
    const std::string wbRels =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
        "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet\" "
        "Target=\"worksheets/sheet1.xml\"/>"
        "</Relationships>";

    std::vector<ZipEntry> files;
    files.push_back({"[Content_Types].xml", contentTypes});
    files.push_back({"_rels/.rels", rels});
    files.push_back({"xl/workbook.xml", workbook});
    files.push_back({"xl/_rels/workbook.xml.rels", wbRels});
    files.push_back({"xl/worksheets/sheet1.xml", sheet});
    return zipWrite(files);
}

} // namespace core
