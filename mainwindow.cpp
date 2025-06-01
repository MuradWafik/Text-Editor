#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "util.h"
#include "editor.h"
#include <QAnyStringView>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow),
    process (new QProcess(this)),
    fileModel(new QFileSystemModel(this))
{
    ui->setupUi(this);

    this->ui->actionSave->setEnabled(false);
    this->setCentralWidget(ui->stackedWidget);
    this->ui->stackedWidget->setCurrentWidget(this->ui->page);
    QDir::setCurrent(QDir::homePath());

    this->setWindowFlags(Qt::WindowStaysOnTopHint);

    setUIChanges();

    // TODO: find alternative
    // ui->plainTextEdit->installEventFilter(this);

    connectSignals();
}

MainWindow::~MainWindow()
{
    if (process->isOpen()) {
        process->write("exit\n");
        process->waitForFinished();
    }

    // deleteAllTabs();

    delete process;
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{

    deleteAllTabs();
    QMainWindow::closeEvent(event);
}


void MainWindow::openFileAction()
{
    // asks to save if they have any changes on current file they are working on before opening dialog
    if(openEditor != nullptr && openEditor->unsavedChanges()){
        QMessageBox::StandardButton saveFileQuestion = QMessageBox::question(this, tr("Save Changes?"),
                                                                             tr("Would you like To Save Changes Before Opening a New Folder?"),
                                                                             QMessageBox::Save | QMessageBox::Discard, QMessageBox::Save);

        if(saveFileQuestion == QMessageBox::Save){
            openEditor->saveFile();
        }
    }

    QString fileName = QFileDialog::getOpenFileName(this, ("Choose File To Open"));

    // setOption(QFileDialog.ReadOnly, true);
    if(openFile(fileName)) getAllFilesInDirectory();
}

void MainWindow::updateStatusBarCursorPosition()
{

    if(openEditor == nullptr) return;
    auto cursor = openEditor->getPte()->textCursor();

    int col = cursor.columnNumber() + 1;
    int line = cursor.blockNumber() + 1;
    QString text = "LN: " + QString::number(line) + ", COL: " + QString::number(col);
    statusBar()->showMessage(text);
    lineAndColStatusLabel->setText(text);
}

void MainWindow::initTerminalBox(const QString& path)
{

    this->ui->terminalBox->clear(); // clears the text in case they are switching files
    // maybe remove, or leave to a setting if they want to

    process->start(util::getShellCommand());

    if (!process->waitForStarted()) {
        QMessageBox::critical(this, tr("Error"), tr("Failed to start the command process"));
        return;
    }

    // there is no open editor if you open a folder
    process->setWorkingDirectory(path);
        // process->setWorkingDirectory(QFileInfo(openEditor->fileName()).absolutePath());

    connect(process, &QProcess::readyReadStandardOutput, this, &MainWindow::on_StdoutAvailable);
    connect(process, &QProcess::readyReadStandardError, this, &MainWindow::on_StderrAvailable); // Connect the error output signal

}


void MainWindow::on_StdoutAvailable()
{ // when it is ready to be read, it reads the output and prints it to the terminal box
    if(!process->isOpen()){
        return;
    }
    QByteArray terminalOutput = process->readAllStandardOutput();
    ui->terminalBox->appendPlainText(terminalOutput);
}

void MainWindow::on_StderrAvailable(){

    if(!process->isOpen()){
        return;
    }
    QByteArray terminalOutput = process->readAllStandardError();
    // outputs the error to the terminal in red
    ui->terminalBox->appendHtml("<span style = 'color: red;'>" + terminalOutput + "</span>");
}


void MainWindow::writeToTerminal()
{
    if (process->isOpen()){
        QByteArray inputByteArray(ui->inputTerminalCommand->text().toUtf8() + "\n") ;
        char *userText = inputByteArray.data();

        process->write(userText); // inputs the user command into the terminal
    }
    ui->inputTerminalCommand->clear(); // clears the input field for the user
}

void MainWindow::setUIChanges()
{
    this->ui->terminalDockWidget->hide();
    this->ui->fileTreeDockWidget->hide();

    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea); // makes the file explorer, whether right or left fill the space instead of the terminal
}

void MainWindow::runButton()
{
    if(process->isOpen()){
        showTerminal();
        QString runPythonCommand = QString("%1 -u \"%2\"").arg(util::getPythonRunCommand(), openEditor->fileName());
        QByteArray runFileCommand(runPythonCommand.toUtf8() + "\n") ;

        process->write(runFileCommand.data()); // inputs the user command into the terminal
        process->waitForBytesWritten();
    }
}

void MainWindow::getAllFilesInDirectory()
{
    QDir directory = QFileInfo(openEditor->fileName()).dir();

    QStringList filePaths(Qt::CaseInsensitive);
    filePaths << "*.py" << "*.txt" << "*.md" << "*.csv";

    fileModel->setNameFilters(filePaths);
    fileModel->setNameFilterDisables(false); // makes the files that arent within filter hidden instead of shown as disabled

    this->ui->fileListTree->setModel(fileModel);
    this->ui->fileListTree->setContextMenuPolicy(Qt::CustomContextMenu); // allows the right click to show custom menu


    fileModel->setRootPath(directory.path());
    ui->fileListTree->setRootIndex(fileModel->index(directory.path()));
}

void MainWindow::getAllFilesInDirectory(const QString &directory)
{

    QStringList filePaths(Qt::CaseInsensitive);
    filePaths << "*.py" << "*.txt" << "*.md" << "*.csv";

    fileModel->setNameFilters(filePaths);
    fileModel->setNameFilterDisables(false); // makes the files that arent within filter hidden instead of shown as disabled

    this->ui->fileListTree->setModel(fileModel);
    fileModel->setRootPath(directory);
    ui->fileListTree->setRootIndex(fileModel->index(directory));
}



bool MainWindow::openFile(const QString &filePath)
{
    // Currently if you delete a file that is opened in another tab, and that tab is not the active one, it deletes, but if you go back to its tab and save, it creates the file again
    if(filePath.isEmpty()) {
        QMessageBox::warning(this,
                             tr("Warning"),
                             tr("Please select a file")
                             );
        return false;
    }

    // despite the editor being the direct child of the tab, putting the flag to seach children only (not recursivly) always results in a nullptr
    const editor* child = this->ui->openEditorsTabWidget->findChild<editor*>(filePath);
    if(child != nullptr){
        return false; // that means it already has a tab open on this file
    }

    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly | QFile::Text)){
        QString errorMessage{"Can Not Open File " + file.errorString()};
        QMessageBox::warning(this, tr("Warning"), tr(errorMessage.toStdString().c_str()));
        return false;
    }

    this->ui->actionSave->setEnabled(true); // if they successfully opened a file, the save button can be used for it

    if(this->ui->stackedWidget->currentIndex() == 0){
        this->ui->stackedWidget->setCurrentIndex(1); // show the page for the editors... not the page saying open a file to begin
    }

    QFileInfo fileDirectory{filePath};
    currentDirectory.setPath(fileDirectory.path());

    editor* nextPage = new editor(this->ui->openEditorsTabWidget, this);
    nextPage->setObjectName(filePath);
    openEditor = nextPage;


    int newTab = this->ui->openEditorsTabWidget->addTab(nextPage, file.fileName());
    this->ui->openEditorsTabWidget->setCurrentIndex(newTab);
    // IM KEEPING THIS COMMENT JUST TO REMIND MYSELF, CHANGE OBJECT NAMES FOR UI, IT WASNT CHANGING TAB CAUSE YOU WERE CALLING IT ON THE TERMINAL TAB WIDGET

    nextPage->openFile(file);

    file.close();

    this->ui->fileTreeDockWidget->showNormal();
    this->ui->terminalDockWidget->showNormal(); // shows both docks, file explorer, and output

    updateTerminalAndOutput(QFileInfo(openEditor->fileName()).absolutePath());
    return true;
}

void MainWindow::updateTerminalAndOutput(const QString& path)
{
    if(process->state() == QProcess::Running){

        process->setWorkingDirectory(path);
    }
    else if(process->state() == QProcess::NotRunning){
        // if the process isnt running, initialize it
        initTerminalBox(path);
    }
}

void MainWindow::openFolderDialog()
{
    // if they try to open folder while working on something that is not saved, it asks to save beforehand
    if(openEditor != nullptr && openEditor->unsavedChanges()){
        QMessageBox::StandardButton saveFileQuestion = QMessageBox::question(this, tr("Save Changes?"), tr("Would you like To Save Changes Before Opening a New Folder?")
                                                                             , QMessageBox::Save | QMessageBox::Discard, QMessageBox::Save);

        // TODO: add save options, if either never save or always open file in same tab call save instead
        if(saveFileQuestion == QMessageBox::Save) openEditor->saveFile();
    }


    QString dir = QFileDialog::getExistingDirectory(this,
                                                    tr("Open Directory"),
                                                    tr("/home"),
                                                    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
                                                    );
    if(dir.isEmpty()){
        QMessageBox::warning(this, tr("Warning"), tr("Unable To Open Folder"));
        return;
    }

    currentDirectory.setPath(dir);

    // if you open a folder, there are no more open tabs, need to clear
    deleteAllTabs();

    this->ui->stackedWidget->setCurrentIndex(1); // sets the page to the text editor page
    this->ui->fileTreeDockWidget->showNormal();
    updateTerminalAndOutput(dir);

    getAllFilesInDirectory(dir);
}

void MainWindow::openFileWhileEditing(const QString& path){
    // checks to see if there are changes with the file before opening
    /*
    if(this->ui->editorWidget->getPte()->document()->isModified()){
        QMessageBox::StandardButton saveQuestion = QMessageBox::question(this, tr("Save?"),
                                                                         tr("Would you like to save changes before switching"),
                                                                         QMessageBox::Save | QMessageBox::Discard, QMessageBox::Save);
        // TODO: See open folder dialog todo
        if(saveQuestion == QMessageBox::Save) saveFile();
    }
    */
    openFile(path);
}


void MainWindow::showCustomContextMenu(const QPoint &pos){
    QModelIndex index = this->ui->fileListTree->indexAt(pos);
    QString fileToOpenPath = fileModel->filePath(index);

    QMenu contextMenu(tr("Context menu"), this);


    QAction newPythonAction(tr("New Python File"), this);
    connect(&newPythonAction, &QAction::triggered, this,[this, pos]{
        createPythonFile(pos);
    });
    contextMenu.addAction(&newPythonAction);

    QAction newTextFileAction(tr("New Text File"), this);
    connect(&newTextFileAction, &QAction::triggered, this,[this, pos]{
        createTextFile(pos);
    });
    contextMenu.addAction(&newTextFileAction);



    QAction deleteAction("Delete", this);
    connect(&deleteAction, &QAction::triggered, this, [this, &fileToOpenPath]() {
        if(openEditor == nullptr) return; // should be disabled here anyways

        if(fileToOpenPath == openEditor->fileName()){
            QMessageBox::warning(this, tr("Error"), tr("Can not delete file that is currently open"));
            return;
        }
        QFile::remove(fileToOpenPath);
    });

    QAction openAction("Open", this);
    connect(&openAction, &QAction::triggered, this, [this, &fileToOpenPath]() { openFileWhileEditing(fileToOpenPath); });

    if (index.isValid()){ // if they clicked on a part that has a file
        // actions that need a file to be clicked on
        contextMenu.addAction(&deleteAction);

        contextMenu.addAction(&openAction);
    }

    contextMenu.exec(this->ui->fileListTree->viewport()->mapToGlobal(pos));
}


void MainWindow::createPythonFile(const QPoint &clickPoint) {

    QPoint globalPos = this->ui->fileListTree->viewport()->mapToGlobal(clickPoint);

    // finds the click point to place it at the same position of the click, while only being parented to the main window
    QPoint mainWindowPos = this->mapFromGlobal(globalPos);

    QString fileDefaultText = "file.py";

    QLineEdit* fileNameLine = new QLineEdit(this);
    fileNameLine->move(mainWindowPos);
    fileNameLine->setText(fileDefaultText);
    fileNameLine->setFixedWidth(150);
    fileNameLine->show();
    fileNameLine->setFocus();

    fileNameLine->setSelection(0, fileDefaultText.length()-3 ); // removes the .py suffix in the default selection so user can auto change file name


    // connects return pressed signal to create the file
    connect(fileNameLine, &QLineEdit::returnPressed, this, [this, fileNameLine](){
        QString fileName = fileNameLine->text();
        if (!fileName.endsWith(".py")) {
            fileName += ".py";
        }

        const QString path = currentDirectory.absolutePath() + "/" + fileName;
        QFile file{path};
        if (!file.open(QIODevice::WriteOnly)) {

            QString error{QString("Unable to open file ") + file.errorString()};
            QMessageBox::warning(this, tr("Error"), error.toStdString().c_str());
            // delete lineedit and return if error opening file
            delete fileNameLine;

        }
        else{
            file.close();
            openFile(path);
            fileNameLine->deleteLater();
        }
    });

    // deletes line edit if user clicks out of it, or if it loses focus in any way
    connect(fileNameLine, &QLineEdit::editingFinished, this, [fileNameLine](){
        fileNameLine->deleteLater();
    });
}

void MainWindow::createTextFile(const QPoint &clickPoint) {
    QPoint globalPos = this->ui->fileListTree->viewport()->mapToGlobal(clickPoint);

    // finds the click point to place it at the same position of the click, while only being parented to the main window
    QPoint mainWindowPos = this->mapFromGlobal(globalPos);
    QString fileDefaultText = "file.txt";

    QLineEdit* fileNameLine = new QLineEdit(this);
    fileNameLine->move(mainWindowPos);
    fileNameLine->setText(fileDefaultText);
    fileNameLine->setFixedWidth(150);
    fileNameLine->show();
    fileNameLine->setFocus();

    fileNameLine->setSelection(0, fileDefaultText.length()-4 ); // removes the .txt suffix in the default selection so user can auto change file name

    // connects return pressed signal to create the file
    connect(fileNameLine, &QLineEdit::returnPressed, this, [this, fileNameLine]() {
        QString fileName = fileNameLine->text();
        if (!fileName.endsWith(".txt")) {
            fileName += ".txt";
        }


        const QString path = currentDirectory.absolutePath() + "/" + fileName;
        QFile file{path};
        if (!file.open(QIODevice::WriteOnly)) {

            QString error{QString("Unable to open file ") + file.errorString()};
            QMessageBox::warning(this, tr("Error"), error.toStdString().c_str());
            // delete lineedit and return if error opening file
            delete fileNameLine;
            return;
        }
        file.close();
        openFile(path);
        fileNameLine->deleteLater();
    });

    // deletes line edit if user clicks out of it, or if it loses focus in any way
    connect(fileNameLine, &QLineEdit::editingFinished, this, [fileNameLine]() {
        fileNameLine->deleteLater();
    });
}


void MainWindow::newPythonFile()
{
    // QString fileName = QFileDialog::
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    tr("Create New Python File"),
                                                    QDir::homePath(),  // Default directory
                                                    tr("Python Files (*.py);;All Files (*)"));

    // do nothing if they cancel, maybe show message warning later on
    if (fileName.isEmpty()) {
        return;
    }

    // makes sure file has python extention
    if (!fileName.endsWith(".py")) {
        fileName += ".py";
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        QString error{"Failed to create the file: " + file.errorString()};
        QMessageBox::warning(this, tr("Error"), tr(error.toStdString().c_str()));
        return;
    }

    file.close();

    // otherwise crash
    if(openFile(fileName)) getAllFilesInDirectory();
}

void MainWindow::connectSignals(){ // relying on the connection of slots that the qt generated on_foo_bar as clangd would say

    // TODO: possibly disconnet and reconnect if they open new file with (new editor object?)
    // connect(openEditor->getPte(), &QPlainTextEdit::cursorPositionChanged, this, &MainWindow::updateStatusBarCursorPosition);


    connect(this->ui->fileListTree, &QWidget::customContextMenuRequested, this, &MainWindow::showCustomContextMenu);
    connect(this->ui->fileListTree, &QTreeView::doubleClicked, this, [this](QModelIndex index){ // pass the same argument as the doubleClick slot has
        QString fileToOpenPath = fileModel->filePath(index);
        openFileWhileEditing(fileToOpenPath);
    });

    // MENU ACTION BAR BUTTONS
    connect(this->ui->actionShow_Terminal, &QAction::triggered, this, &MainWindow::showTerminal);
    connect(this->ui->actionNew_Text_File, &QAction::triggered, this, &MainWindow::newTextFile);
    connect(this->ui->actionNew, &QAction::triggered, this, &MainWindow::newPythonFile);

    connect(this->ui->actionOpen_File, &QAction::triggered, this, &MainWindow::openFileAction);
    connect(this->ui->actionOpen_Folder, &QAction::triggered, this, &MainWindow::openFolderDialog);

    connect(this->ui->actionSelect_All, &QAction::triggered, this, []{
        // While the check of current file might be useful like in undo, AFAIK, no harm in allowing this (in any case where text may exist out of file)
        // because of this doesnt need "this" in the capture
        QWidget* focusedWidget = QApplication::focusWidget();
        if (auto plainTextEdit = qobject_cast<QPlainTextEdit*>(focusedWidget)) {
            plainTextEdit->selectAll();
        }
        else if (auto lineEdit = qobject_cast<QLineEdit*>(focusedWidget)) {
            lineEdit->selectAll();
        } // tries to cast the focused widget into one of these, if possible calls on the built in select all function

    });
    connect(this->ui->actionUndo, &QAction::triggered, this, []{

        // if(openEditor == nullptr) return;
        QWidget* focusedWidget = QApplication::focusWidget();
        if (auto plainTextEdit = qobject_cast<QPlainTextEdit*>(focusedWidget)) {
            plainTextEdit->undo();
        }
        else if (auto lineEdit = qobject_cast<QLineEdit*>(focusedWidget)) {
            lineEdit->undo();
        } // tries to cast the focused widget into one of these, if possible calls on the built in undo function
    });

    connect(this->ui->actionRedo, &QAction::triggered, this, []{
        // if(currentFile.isEmpty()) return;
        QWidget* focusedWidget = QApplication::focusWidget();

        if (auto plainTextEdit = qobject_cast<QPlainTextEdit*>(focusedWidget)) {
            plainTextEdit->redo();
        }
        else if (auto lineEdit = qobject_cast<QLineEdit*>(focusedWidget)) {
            lineEdit->redo();

        } //  just like undo
        //tries to cast the focused widget into one of these, if possible calls on the built in redo function
    });


    connect(this->ui->actionSave, &QAction::triggered, this, [this]{
        this->openEditor->saveFile();
    });


    connect(this->ui->actionHide_Terminal, &QAction::triggered, this, [this]{
        this->ui->terminalDockWidget->hide();
    });
    connect(this->ui->actionClear_Terminal, &QAction::triggered, this, [this]{
        this->ui->terminalBox->clear();
    });

    connect(this->ui->actionShow_File_Tree, &QAction::triggered, this, [this]{
        this->ui->fileTreeDockWidget->showNormal();
    });
    connect(this->ui->actionFind_Replace, &QAction::triggered, this, [this]{
        if(openEditor == nullptr) return;
        openEditor->showSearchAndReplace();
    });

    // END OF MENU BAR ACTIONS

    connect(this->ui->runFileButton, &QPushButton::pressed, this, &MainWindow::runButton);
    connect(this->ui->inputTerminalCommand, &QLineEdit::returnPressed, this, &MainWindow::writeToTerminal);

    connect(this->ui->openEditorsTabWidget, &QTabWidget::currentChanged, this, [this]{
        openEditor = qobject_cast<editor*>(ui->openEditorsTabWidget->currentWidget());
    });

    connect(this->ui->openEditorsTabWidget, &QTabWidget::tabCloseRequested, this, [this](int index){
        //delete openEditor; // call on its destructor which manages choices regarding save

        // this->ui->openEditorsTabWidget->removeTab(index);
        // for some reason, removing the tab through the intended method does not manage its memory, but also does deletes the tab to its right if you try to manage the memory

        // to negate the issue mentioned above, it sets the pointer to the intended tab to be closed, and deletes that
        openEditor = qobject_cast<editor*>(this->ui->openEditorsTabWidget->widget(index));
        openEditor->deleteLater();
        openEditor = qobject_cast<editor*>(this->ui->openEditorsTabWidget->currentWidget());
        // on closing a tab, delete a pointer (it manages its own data), and change the pointer to the current open tab
    });
}


void MainWindow::newTextFile()
{
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    tr("Create New Text File File"),
                                                    QDir::homePath(),  // default directory
                                                    tr("Text Files (*.txt);;All Files (*)")
                                                    );

    // do nothing if they cancel, maybe show message warning later on
    if (fileName.isEmpty()){
        return;
    }

    // makes sure file has txt extention
    if (!fileName.endsWith(".txt")){
        fileName += ".txt";
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)){
        QString error{"Failed to create the file: "+ file.errorString()};
        QMessageBox::warning(this, tr("Error"), tr(error.toStdString().c_str()));
        return;
    }

    file.close();


    if(openFile(fileName)) getAllFilesInDirectory();
}

void MainWindow::showTerminal(){
    this->ui->terminalDockWidget->showNormal(); // if they press new terminal, it shows the widget
}

void MainWindow::deleteAllTabs(){
    auto tabWidget = this->ui->openEditorsTabWidget;
    while(tabWidget->count() != 0){
        editor* cur = qobject_cast<editor*>(tabWidget->widget(0));
        tabWidget->removeTab(0);
        // cur->deleteLater();
        delete cur;
    }

    // it should be deleted by now, but setting it to nullptr for any checks that occur elsewhere
    openEditor = nullptr;
}
