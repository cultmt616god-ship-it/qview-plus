#ifndef QVAIRENAMEDIALOG_H
#define QVAIRENAMEDIALOG_H

#include <QDialog>
#include <QFileInfo>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QProgressBar>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QFile>
#include <QByteArray>

class QVAIRenameDialog : public QDialog
{
    Q_OBJECT

public:
    QVAIRenameDialog(QWidget *parent, QFileInfo fileInfo);

signals:
    void newFileToOpen(const QString &filePath);
    void readyToRenameFile();

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void onGenerateClicked();
    void onRenameClicked();
    void onCancelClicked();
    void onNetworkReply(QNetworkReply *reply);

private:
    void generateAIName();
    void performRename();
    void loadSettings();
    void saveSettings();
    
    QFileInfo fileInfo;
    QLineEdit *fileNameEdit;
    QPushButton *generateButton;
    QPushButton *renameButton;
    QPushButton *cancelButton;
    QPushButton *settingsButton;
    QTextEdit *descriptionEdit;
    QLabel *statusLabel;
    QProgressBar *progressBar;
    QNetworkAccessManager *networkManager;
    
    QString suggestedName;
};

#endif // QVAIRENAMEDIALOG_H