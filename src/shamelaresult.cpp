#include "shamelaresult.h"
#include "common.h"

ShamelaResult::ShamelaResult()
{
    m_edited = true;
}

ShamelaResult::~ShamelaResult()
{
}

QString ShamelaResult::text()
{
    return m_text;
}

QString ShamelaResult::title()
{
    return m_title;
}

QString ShamelaResult::snippet()
{
    return m_snippet;
}

QString ShamelaResult::bgColor()
{
    return m_bgColor;
}

QString ShamelaResult::bookInfo()
{
    return m_info;
}

QString ShamelaResult::toHtml()
{
    if(m_edited) {
        m_html = QString::fromUtf8("<div class='result %1' bookid='%4' rid='%5' pid='%11'>"
                                   "<div class='result_head'>"
                                   "<h3>%2</h3>"
                                   "<span class='progSpan' style='width: %3px;'>"
                                   "<span class='progSpanContainre'></span>"
                                   "</span>"
                                   "</div>"
                                   "<p class='resultText' bookid='%4' rid='%5'>%6</p>"
                                   "<p class='result_info' bookid='b%4'> كتاب: <span class='bookName' title='%10'>%7</span>"
                                   "<span class='shamela_tools'>"
                                        "<span class='shamela_open'><img src='qrc:/data/images/shamela.png' /></span>"
                                        "<span class='viewer_open'><img src='qrc:/data/images/shamela-viewer.png' /></span>"
                                   "</span>"
                                   "<span style='float: left;margin: 5px 0px'>الصفحة: <span style='margin-left: 7px;'>%8</span>  الجزء: <span>%9</span></span>"
                                   "</p></div>")
                .replace('\'', '"')
                .arg(m_bgColor)                 // backround class name (%1)
                .arg(m_title)                   // bab (%2)
                .arg(m_score)                   // score (%3)
                .arg(m_bookId)                  // Book id (%3)
                .arg(m_id)                      // Result id (%3)
                .arg(m_snippet.simplified())    // text snippet (%6)
                .arg(m_bookName)                // book name (%7)
                .arg(m_page)                    // page (%8)
                .arg(m_part)                    // part (%9)
                .arg(m_info.replace('\"', '\'').replace(QRegExp("[\\r\\n]+"), " - ")) // book info(%10)
                .arg(m_pageID); // Page Id (%11)

        m_edited = false;
    }

    return m_html;
}

int ShamelaResult::page()
{
    return m_page;
}

int ShamelaResult::part()
{
    return m_part;
}

int ShamelaResult::pageID()
{
    return m_pageID;
}

int ShamelaResult::bookId()
{
    return m_bookId;
}

int ShamelaResult::archive()
{
    return m_archive;
}

int ShamelaResult::score()
{
    return m_score;
}

void ShamelaResult::setText(const QString &text)
{

    if(m_text != text) {
        m_text = text;
        clearShorts(m_text);

        m_edited = true;
    }
}

void ShamelaResult::setTitle(const QString &title)
{

    if(m_title != title) {
        m_title = title;
        m_edited = true;
    }
}

void ShamelaResult::setSnippet(const QString &snippet)
{
    if(m_snippet != snippet) {
        m_snippet = snippet;

        m_edited = true;
    }
}

void ShamelaResult::setBgColor(const QString &color)
{

    if(m_bgColor != color) {
        m_bgColor = color;
        m_edited = true;

    }
}

void ShamelaResult::setBookName(const QString &name)
{
    if(name != m_bookName) {
        m_bookName = name;
        m_edited = true;
    }
}

void ShamelaResult::setBookInfo(const QString &info)
{
    if(info != m_info) {
        m_info = info;
        m_edited = true;
    }
}

void ShamelaResult::setPage(int page)
{

    if(m_page != page) {
        m_page = page;
        m_edited = true;
    }
}

void ShamelaResult::setPart(int part)
{
    if(m_part != part){
        m_part = part;
        m_edited = true;
    }
}

void ShamelaResult::setPageId(int id)
{
    if(m_pageID != id) {
        m_pageID = id;
        m_edited = true;
    }
}

void ShamelaResult::setBookId(int id)
{
    if(m_bookId != id) {
        m_bookId = id;
        m_edited = true;
    }
}

void ShamelaResult::setArchive(int archive)
{
    if(m_archive != archive) {
        m_archive = archive;
        m_edited = true;
    }
}

void ShamelaResult::setScore(int score)
{
    if(m_score != score) {
        m_score = score;
        m_edited = true;
    }
}

void ShamelaResult::setId(int id)
{
    if(m_id != id) {
        m_id = id;
        m_edited = true;
    }
}

int ShamelaResult::id()
{
    return m_id;
}
