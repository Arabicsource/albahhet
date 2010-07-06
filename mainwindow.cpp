#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
        QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("CLucene Test");

    m_resultModel = new QStandardItemModel();
    m_results = new Results();
    m_resultCount = 0;
    m_resultParPage = 10;
    m_groupSize = 550;
    m_dbIsOpen = false;

    m_colors.append("#FFFF63");
    m_colors.append("#A5FFFF");
    m_colors.append("#FF9A9C");
    m_colors.append("#9CFF9C");
    m_colors.append("#EF86FB");

    QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                       "cluceneTest", "cluceneTestX");
    m_bookPath = settings.value("db").toString();
    if(!m_bookPath.endsWith('/'))
        m_bookPath.append('/');
    m_searchQuery = settings.value("lastQuery").toString();
    m_resultParPage = settings.value("resultPeerPage", m_resultParPage).toInt();
    m_groupSize = settings.value("groupSize", m_groupSize).toInt();
    ui->lineBook->setReadOnly(true);
    ui->lineBook->setText(m_bookPath);
    ui->lineQuery->setText(m_searchQuery);

    ui->webView->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);

    connect(ui->pushIndex, SIGNAL(clicked()), this, SLOT(startIndexing()));
    connect(ui->pushSearch, SIGNAL(clicked()), this, SLOT(startSearching()));
    connect(ui->pushStatstic, SIGNAL(clicked()), this, SLOT(showStatistic()));
    connect(ui->webView, SIGNAL(linkClicked(QUrl)), this, SLOT(resultLinkClicked(QUrl)));
}

MainWindow::~MainWindow()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                       "cluceneTest", "cluceneTestX");
    settings.setValue("db", m_bookPath);
    settings.setValue("lastQuery", ui->lineQuery->text());
    settings.setValue("resultPeerPage", m_resultParPage);
    settings.setValue("groupSize", m_groupSize);
    delete ui;
}

void MainWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void MainWindow::startIndexing()
{
    int const MAX_GROUP_SIZE = m_groupSize * 1024 * 1024;
    int currentGroup = 1;
    int currentGroupSize = 0;
    int bookSize = 0;
    int totalBooksSize = 0;
    if(!m_dbIsOpen)
        openDB();
    {
        QSqlDatabase indexDB = QSqlDatabase::addDatabase("QSQLITE", "bookIndex");
        indexDB.setDatabaseName("book_index.db");
        if(!indexDB.open())
            qDebug("Error opning index db");
        QSqlQuery *inexQuery = new QSqlQuery(indexDB);
        inexQuery->exec("CREATE TABLE IF NOT EXISTS books (id INTEGER PRIMARY KEY, bookName TEXT, "
                        "shamelaID INTEGER, filePath TEXT, authorId INTEGER, authorName TEXT, "
                        "fileSize INTEGER, cat INTEGER)");
        inexQuery->exec("DELETE FROM books");


        indexDB.transaction();
        m_bookQuery->exec("SELECT Bk, bkid, auth, authno FROM 0bok WHERE Archive = 0");
        while(m_bookQuery->next()) {
            QString bookPath = buildFilePath(m_bookQuery->value(1).toString());
            QFile bookFile(bookPath);
            if(bookFile.exists()){
                bookSize = bookFile.size();
                if(currentGroupSize+bookSize < MAX_GROUP_SIZE)
                    currentGroupSize += bookSize;
                else {
                    currentGroup++;
                    currentGroupSize = 0;
                }
                totalBooksSize += bookSize;
                if(!inexQuery->exec(QString("INSERT INTO books VALUES (NULL, '%1', %2, '%3', %4, '%5', '%6', '%7')")
                    .arg(m_bookQuery->value(0).toString())
                    .arg(m_bookQuery->value(1).toString())
                    .arg(bookPath)
                    .arg(m_bookQuery->value(3).toString())
                    .arg(m_bookQuery->value(2).toString())
                    .arg(bookSize)
                    .arg(currentGroup)))
                    qDebug()<< "ERROR:" << inexQuery->lastError().text();
            } else
                qDebug()<< "NOT FOUND:" << bookPath;
        }
        indexDB.commit();
    }
    ui->statusBar->showMessage(trUtf8("حجم المكتبة: %1 عدد المجموعات: %2").arg(totalBooksSize).arg(currentGroup));
    QSqlDatabase::removeDatabase("bookIndex");
    IndexingDialg *indexDial = new IndexingDialg(this);
    indexDial->setModal(true);
    indexDial->show();
}

void MainWindow::startSearching()
{
    m_resultModel->clear();
    if(!m_dbIsOpen)
        openDB();
    try {
        ArabicAnalyzer analyzer;
        m_searchQuery = ui->lineQuery->text();

        m_searchQuery.replace(QRegExp(trUtf8("ـفق")), "(");
        m_searchQuery.replace(QRegExp(trUtf8("ـغق")), ")");
        m_searchQuery.replace(QRegExp(trUtf8("ـ[أا]و")), "OR");
        m_searchQuery.replace(QRegExp(trUtf8("ـو")), "AND");
        m_searchQuery.replace(QRegExp(trUtf8("ـبدون")), "NOT");
        m_searchQuery.replace(trUtf8("؟"), "?");

        QString queryWord = m_searchQuery;
        IndexSearcher *searcher = new IndexSearcher(INDEX_PATH);

        // Start building the query
        QueryParser *queryPareser = new QueryParser(_T("text"),&analyzer);
        if(ui->checkBox->isChecked())
            queryPareser->setDefaultOperator(QueryParser::AND_OPERATOR);
        queryPareser->setAllowLeadingWildcard(true);

        Query* q = queryPareser->parse(QSTRING_TO_TCHAR(queryWord));
//        qDebug() << "Search: " << TCHAR_TO_QSTRING(q->toString(_T("text")));
//        qDebug() << "Query : " << queryWord;

        QTime time;

        time.start();
        m_results->setHits(searcher->search(q));
        int timeSearch = time.elapsed();

        m_resultCount = m_results->resultsCount();
        m_results->setPageCount(_toBInt((m_results->resultsCount()/(double)m_resultParPage)));
        m_results->setCurrentPage(0);
        displayResults();

        this->statusBar()->showMessage(trUtf8("تم البحث خلال %1 "SECONDE_AR".  "
                                               "نتائج البحث %2")
                                       .arg(miTOsec(timeSearch))
                                       .arg(m_resultCount));
//        _CLLDELETE(hits);
//        _CLLDELETE(q);
//        searcher->close();
    }

    catch(CLuceneError &tmp) {
        QMessageBox::warning(0, "Error search", tmp.what());
    }

    catch(...) {
        qDebug() << "Error when searching at : " << INDEX_PATH;
    }

}

void MainWindow::showStatistic()
{
    try {
        IndexReader* r = IndexReader::open(INDEX_PATH);
        int64_t ver = r->getCurrentVersion(INDEX_PATH);

        QString txt;
        txt.append(tr("Statistics for <strong>%1</strong><br/>").arg(INDEX_PATH));
        //txt.append(tr("Max Docs: %1<br/>").arg(r->maxDoc()));
        txt.append(tr("Current Version: %1<br/><br/>").arg(ver)) ;
        txt.append(tr("Num Docs: %1<br/>").arg(r->numDocs()));

        TermEnum* te = r->terms();
        int32_t nterms;
        for (nterms = 0; te->next() == true; nterms++) {
            /* qDebug() << TCHAR_TO_QSTRING(te->term()->text()); */
        }
        txt.append(tr("Term count: %1<br/>").arg(nterms));
        txt.append(tr("Index size: %1<br/>").arg(getIndexSize()));
        txt.append(tr("Book size: %1").arg(getBookSize()));
        _CLLDELETE(te);

        r->close();
        _CLLDELETE(r);
        QMessageBox::information(0, "Statistics", txt);
    }
    catch(CLuceneError &err) {
        QMessageBox::warning(0, "Error search", err.what());
    }
    catch(...) {
        qDebug() << "Error when opening : " << INDEX_PATH;
    }

}

QString MainWindow::cleanString(QString str)
{
    str.replace(QRegExp("[\\x0627\\x0622\\x0623\\x0625]"), "[\\x0627\\x0622\\x0623\\x0625]");//ALEFs
    str.replace(QRegExp("[\\x0647\\x0629]"), "[\\x0647\\x0629]"); //TAH_MARBUTA -> HEH

    return str;
}

void MainWindow::resultsCount()
{
    this->statusBar()->showMessage(trUtf8("نتائج البحث %1")
                                   .arg(m_resultCount));
}

void MainWindow::displayResults(/*result &pResult*/)
{

    QString resultString;

    int start = m_results->currentPage() * m_resultParPage;
    int maxResult  =  (m_results->resultsCount() >= start+m_resultParPage)
                      ? start+m_resultParPage : m_results->resultsCount();
    bool whiteBG = true;
    int entryID;
    for(int i=start; i < maxResult;i++){
        {
            int bookID = m_results->bookIdAt(i);
            QSqlDatabase m_bookDB = QSqlDatabase::addDatabase("QODBC", "resultBook");
            QString mdbpath = QString("DRIVER={Microsoft Access Driver (*.mdb)};FIL={MS Access};DBQ=%1")
                              .arg(buildFilePath(QString::number(bookID)));
            m_bookDB.setDatabaseName(mdbpath);

            if (!m_bookDB.open()) {
                qDebug() << "Cannot open" << buildFilePath(QString::number(bookID)) << "database.";
            }
            QSqlQuery *m_bookQuery = new QSqlQuery(m_bookDB);

            entryID = m_results->idAt(i);
            m_bookQuery->exec(QString("SELECT nass, page, part FROM book WHERE id = %1")
                              .arg(entryID));
            if(m_bookQuery->first()){
                resultString.append(trUtf8("<div style=\"margin: 0px; padding: 4px; border-bottom: 1px solid black; background-color: %1 ;\">"
                                           "<h3 style=\"margin: 0px 0px 5px; font-size: 18px; color: rgb(0, 68, 255);\">%2</h3>"
                                           "<a style=\"margin: 0px; padding: 0px 5px;\" href=\"http://localhost/book.html?id=%3&bookid=%8\">%4</a>"
                                           "<p style=\"margin: 5px 0px 0px;\"> كتاب: <span style=\"margin-left: 7px; color: green; font-weight: bold;\">%5</span>"
                                           "<span style=\"float: left;\">الصفحة: <span style=\"margin-left: 7px;\">%6</span>  الجزء: <span>%7</span></span>"
                                           "</p></div>")
                                    .arg(whiteBG ? "#FFFFFF" : "#EFEFEF") //BG color
                                    .arg(getTitleId(entryID)) //BAB
                                    .arg(entryID) // entry id
                                    .arg(hiText(abbreviate(m_bookQuery->value(0).toString(),320), m_searchQuery)) // TEXT
                                    .arg(getBookName(bookID)) // BOOK_NAME
                                    .arg(m_bookQuery->value(1).toString()) // PAGE
                                    .arg(m_bookQuery->value(2).toString()) // PART
                                    .arg(bookID));
                whiteBG = !whiteBG;
            }
        }
        QSqlDatabase::removeDatabase("resultBook");
    }

    setPageCount(m_results->currentPage()+1, m_results->pageCount());
    ui->webView->setHtml("<html><head><style>a{text-decoration: none;color:#000000;} a:hover {color:blue}</style></head>"
                         "<body style=\"direction: rtl; margin: 0pt; padding: 2px;\">"+resultString+"</body></html>");

}

void MainWindow::on_pushGoNext_clicked()
{
    m_results->setCurrentPage(m_results->currentPage()+1);
    displayResults();
}

void MainWindow::on_pushGoPrev_clicked()
{
    m_results->setCurrentPage(m_results->currentPage()-1);
    displayResults();
}

void MainWindow::on_pushGoFirst_clicked()
{
    m_results->setCurrentPage(0);
    displayResults();
}

void MainWindow::on_pushGoLast_clicked()
{
    m_results->setCurrentPage(m_results->pageCount()-1);
    displayResults();
}

QString MainWindow::hiText(const QString &text, const QString &strToHi)
{
    QStringList regExpStr = buildRegExp(strToHi);
    QString finlStr  = text;
    int color = 0;
    bool useColors = (regExpStr.size() <= m_colors.size());

    foreach(QString regExp, regExpStr)
        finlStr.replace(QRegExp(cleanString(regExp)),
                        QString("<b style=\"background-color:%1\">\\1</b>")
                        .arg(m_colors.at(useColors ? color++ : color)));

//    if(!useColors)
//        finlStr.replace(QRegExp("<\\/b>([\\s])<b style=\"background-color:[^\"]+\">"), "\\1");

    return finlStr;
}

QStringList MainWindow::buildRegExp(const QString &str)
{
    QStringList strWords = str.split(QRegExp(trUtf8("[\\s;,.()\"{}\\[\\]]")), QString::SkipEmptyParts);
    QStringList regExpList;
    foreach(QString word, strWords)
    {
        QString regExpStr;
        regExpStr.append("\\b(");

        for (int i=0; i< word.size();i++) {
            if(word.at(i) == QChar('~'))
                regExpStr.append("[\\S]*");
            else if(word.at(i) == QChar('*'))
                regExpStr.append("[\\S]*");
            else if(word.at(i) == QChar('?'))
                regExpStr.append("\\S");
            else {
                regExpStr.append(word.at(i));
                regExpStr.append(trUtf8("[ًٌٍَُِّْ]*"));
            }
        }
        regExpStr.append(")\\b");
        regExpList.append(regExpStr);
    }
    return regExpList;
}

void MainWindow::setPageCount(int current, int count)
{
    int rCount = _atLeastOne(count);
    int rCurrent = _atLeastOne(current);
    ui->label->setText(trUtf8("الصفحة %1 / %2")
                       .arg(rCount)
                       .arg(rCurrent));
    buttonStat(rCurrent, rCount);
}

void MainWindow::buttonStat(int currentPage, int pageCount)
{
    if(currentPage == 1) {
        ui->pushGoPrev->setEnabled(false);
        ui->pushGoFirst->setEnabled(false);
    } else {
        ui->pushGoPrev->setEnabled(true);
        ui->pushGoFirst->setEnabled(true);
    }

    if(currentPage == pageCount){
        ui->pushGoNext->setEnabled(false);
        ui->pushGoLast->setEnabled(false);
    } else {
        ui->pushGoNext->setEnabled(true);
        ui->pushGoLast->setEnabled(true);
    }


}

void MainWindow::on_lineQuery_returnPressed()
{
    startSearching();
}

QStringList MainWindow::makeVLabels(int start, int end)
{
    QStringList labls;
    for(int i=start+1; i<=end;i++)
        labls.append(QString::number(i));
    return labls;
}

void MainWindow::on_pushButton_clicked()
{
    QString path = QFileDialog::getExistingDirectory(this,
                                 trUtf8("اختر مجلد المكتبة الشاملة"));
    if(!path.isEmpty()) {
        m_bookPath = path;
        if(!m_bookPath.endsWith('/'))
            m_bookPath.append('/');
        ui->lineBook->setText(m_bookPath);
        if(QSqlDatabase::contains("shamelaBooks"))
            QSqlDatabase::removeDatabase("shamelaBooks");
        if(openDB()){
            int rep = QMessageBox::question(this,
                                            trUtf8("فهرسة المكتبة"),
                                            trUtf8("هل تريد فهرسة المكتبة ؟"),
                                            QMessageBox::Yes|QMessageBox::No);
            if(rep==QMessageBox::Yes)
                startIndexing();
        }
    }
}

bool MainWindow::openDB()
{
    m_dbIsOpen = false;
    QString book = m_bookPath;
    book.append("Files/main.mdb");

    if(!QFile::exists(book)) {
        QMessageBox::warning(this,
                             trUtf8("خطأ في اختيار مجلد الشاملة"),
                             trUtf8("المرجوا اختيار مجلد المكتبة الشاملة"));
        return false;
    }
    m_bookDB = QSqlDatabase::addDatabase("QODBC", "shamelaBook");
    QString mdbpath = QString("DRIVER={Microsoft Access Driver (*.mdb)};FIL={MS Access};DBQ=%1").arg(book);
    m_bookDB.setDatabaseName(mdbpath);

    if (!m_bookDB.open()) {
        QMessageBox::warning(0, "Error opening database", "Cannot open main.mdb database.");
        return false;
    }
    m_bookQuery = new QSqlQuery(m_bookDB);
    m_dbIsOpen = true;

    return true;
}

QString MainWindow::abbreviate(QString str, int size) {
        if (str.length() <= size-3)
                return str;
        str.simplified();
        int index = str.lastIndexOf(' ', size-3);
        if (index <= -1)
                return "";
        return str.left(index).append("...");
}

QString MainWindow::getIndexSize()
{
    QDir dir;
    dir.cd(INDEX_PATH);
    dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
    dir.setSorting(QDir::Size | QDir::Reversed);

    QFileInfoList list = dir.entryInfoList();
    QString sizeStr;
    int size = 0;
    for (int i = 0; i < list.size(); ++i) {
        QFileInfo fileInfo = list.at(i);
        size += fileInfo.size();
    }

    if(size <= 1024)
        sizeStr = tr("%1 B").arg(size);
    else if(1024 <= size && size <= 1024*1024)
        sizeStr = tr("%1 KB").arg(size/(1024.0), 4);
    else if(size >= 1024*1024)
        sizeStr = tr("%1 MB").arg(size/(1024.0*1024.0), 4);
    else
        sizeStr = tr("%1 GB").arg(size);

    int bookSize = 0;
    {
        QSqlDatabase indexDB = QSqlDatabase::addDatabase("QSQLITE", "bookSize");
        indexDB.setDatabaseName("book_index.db");
        if(!indexDB.open())
            qDebug("Error opning index db");
        QSqlQuery *inexQuery = new QSqlQuery(indexDB);
        inexQuery->exec("SELECT fileSize FROM books");
        while(inexQuery->next())
            bookSize += inexQuery->value(0).toInt();
    }
    QSqlDatabase::removeDatabase("bookSize");

    sizeStr.append(tr(" (%1%)").arg(((double)size/(double)bookSize)*100.0, 2));
    return sizeStr;
}

QString MainWindow::getBookSize()
{

    int size = 0;
    {
        QSqlDatabase indexDB = QSqlDatabase::addDatabase("QSQLITE", "bookSize");
        indexDB.setDatabaseName("book_index.db");
        if(!indexDB.open())
            qDebug("Error opning index db");
        QSqlQuery *inexQuery = new QSqlQuery(indexDB);
        inexQuery->exec("SELECT fileSize FROM books");
        while(inexQuery->next())
            size += inexQuery->value(0).toInt();
    }
    QSqlDatabase::removeDatabase("bookSize");

    if(size <= 1024)
        return tr("%1 B").arg(size);
    else if(1024 <= size && size <= 1024*1024)
        return tr("%1 KB").arg(size/(1024.0), 4);
    else if(size >= 1024*1024)
        return tr("%1 MB").arg(size/(1024.0*1024.0), 4);
    else
        return tr("%1 GB").arg(size);
}

void MainWindow::writeLog(int indexingTime)
{
    try {
        IndexReader* r = IndexReader::open(INDEX_PATH);
        int64_t ver = r->getCurrentVersion(INDEX_PATH);

        QString txt;
        txt.append(tr("[+] Date: %1\n").arg(QDateTime::currentDateTime().toString("dd/MM/yyyy - HH:MM:ss")));
#ifdef GITVERSION
        txt.append(tr("[+] Git: %1 - Change number: %2\n").arg(GITVERSION).arg(GITCHANGENUMBER));
#endif
        txt.append(tr("[+] Statistics for \"%1\"\n").arg(INDEX_PATH));
        txt.append(tr("[+] Book: \"%1\"\n").arg(m_bookPath));
        txt.append(tr("[+] Current Version: %1\n").arg(ver)) ;
        txt.append(tr("[+] Num Docs: %1\n").arg(r->numDocs()));

        TermEnum* te = r->terms();
        int32_t nterms;
        for (nterms = 0; te->next() == true; nterms++) {
            /* qDebug() << TCHAR_TO_QSTRING(te->term()->text()); */
        }
        txt.append(tr("[+] Indexing took: %1 s\n").arg(miTOsec(indexingTime)));
        txt.append(tr("[+] Term count: %1\n").arg(nterms));
        txt.append(tr("[+] Index size: %1\n").arg(getIndexSize()));
        txt.append(tr("[+] Book size: %1\n").arg(getBookSize()));
        txt.append("===========================================\n\n");
        _CLLDELETE(te);

        r->close();
        _CLLDELETE(r);

        QFile logFile("statistic.txt");
        if(logFile.open(QIODevice::Append)) {
             QTextStream log(&logFile);
             log << txt;
        }

    }
    catch(...) {}

}

QString MainWindow::getTitleId(int pageID)
{
    QSqlQuery *m_bookQuery = new QSqlQuery(QSqlDatabase::database("resultBook"));
    m_bookQuery->exec(QString("SELECT TOP 1 tit FROM title WHERE id <= %1 ORDER BY id DESC").arg(pageID));

    if(m_bookQuery->first())
        return m_bookQuery->value(0).toString();
    else {
        qDebug() << m_bookQuery->lastError().text();
        return m_bookName;
    }

}

QString MainWindow::getBookName(int bookID)
{
    QSqlQuery *m_bookQuery = new QSqlQuery(QSqlDatabase::database("shamelaBook"));
    m_bookQuery->exec(QString("SELECT bk FROM 0bok WHERE bkid = %1").arg(bookID));

    if(m_bookQuery->first())
        return m_bookQuery->value(0).toString();
    else {
        qDebug() << m_bookQuery->lastError().text();
        return m_bookName;
    }

}

void MainWindow::resultLinkClicked(const QUrl &url)
{
    int rid = url.queryItems().first().second.toInt();
    int bookID = url.queryItems().last().second.toInt();
    QDialog *dialog = new QDialog(this);
    QVBoxLayout *layout= new QVBoxLayout(dialog);
    QTextBrowser *textBrowser = new QTextBrowser(0);
    QString text;
    layout->addWidget(textBrowser);
    {
        QSqlDatabase m_bookDB = QSqlDatabase::addDatabase("QODBC", "disBook");
        QString mdbpath = QString("DRIVER={Microsoft Access Driver (*.mdb)};FIL={MS Access};DBQ=%1")
                          .arg(buildFilePath(QString::number(bookID)));
        m_bookDB.setDatabaseName(mdbpath);

        if (!m_bookDB.open()) {
            qDebug() << "Cannot open" << buildFilePath(QString::number(bookID)) << "database.";
        }
        QSqlQuery *m_bookQuery = new QSqlQuery(m_bookDB);

        m_bookQuery->exec(QString("SELECT page, part, nass FROM book WHERE id = %2")
                          .arg(rid));
        if(m_bookQuery->first())
            text = m_bookQuery->value(2).toString();

        text.replace(QRegExp("[\\r\\n]"),"<br/>");
    }
    QSqlDatabase::removeDatabase("disBook");

    textBrowser->setHtml(hiText(text, m_searchQuery));
    textBrowser->setAlignment(Qt::AlignRight);

    dialog->setLayout(layout);
    dialog->resize(500,500);
    dialog->show();
}

void MainWindow::on_pushButton_2_clicked()
{
    QDialog *settingDialog =  new QDialog(this);
    QVBoxLayout *vLayout= new QVBoxLayout(settingDialog);

    QHBoxLayout *hLayout= new QHBoxLayout(settingDialog);
    QLabel *label = new QLabel(trUtf8(":عدد النتائج في كل صفحة"), settingDialog);
    QSpinBox *spinPage = new QSpinBox(settingDialog);
    QPushButton *pushDone = new QPushButton(trUtf8("حفظ"), settingDialog);

    spinPage->setMaximum(1000);
    spinPage->setMinimum(1);
    spinPage->setValue(m_resultParPage);
    hLayout->addWidget(spinPage);
    hLayout->addWidget(label);

    QHBoxLayout *hLayout2= new QHBoxLayout(settingDialog);
    QLabel *label2 = new QLabel(trUtf8("حجم كل مجموعة"), settingDialog);
    QSpinBox *spinPage2 = new QSpinBox(settingDialog);

    spinPage2->setMaximum(1000);
    spinPage2->setMinimum(1);
    spinPage2->setSuffix(" MB");
    spinPage2->setValue(m_groupSize);
    hLayout2->addWidget(spinPage2);
    hLayout2->addWidget(label2);

    vLayout->addLayout(hLayout);
    vLayout->addLayout(hLayout2);
    vLayout->addWidget(pushDone);
    settingDialog->setLayout(vLayout);
    settingDialog->show();

    connect(pushDone, SIGNAL(clicked()), settingDialog, SLOT(accept()));
    connect(spinPage, SIGNAL(valueChanged(int)), this, SLOT(setResultParPage(int)));
    connect(spinPage2, SIGNAL(valueChanged(int)), this, SLOT(setGroupSize(int)));
}

QString MainWindow::buildFilePath(QString bkid)
{
    return QString("%1Books/%2/%3.mdb").arg(m_bookPath).arg(bkid.right(1)).arg(bkid);
}

void MainWindow::on_pushButton_3_clicked()
{
    QDir dir;
    foreach(QString childDir, dir.entryList(QDir::NoDotAndDotDot|QDir::Dirs)) {
        if(childDir.contains(QRegExp("(index_book|tmp_[a-f0-9]*)"))) {
            QDir indexDir(childDir);
            foreach(QString file, indexDir.entryList(QDir::NoDotAndDotDot|QDir::Files))
                indexDir.remove(file);
            indexDir.cdUp();
            indexDir.rmdir(childDir);
        }
    }
    statusBar()->showMessage(trUtf8("تم حذف الفهارس"),
                             3000);
}
