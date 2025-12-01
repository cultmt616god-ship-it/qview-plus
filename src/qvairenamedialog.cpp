#include "qvairenamedialog.h"
#include "qvapplication.h"

#include <QDir>
#include <QMessageBox>
#include <QSettings>
#include <QApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QTimer>
#include <QDateTime>
#include <QMessageBox>
#include <QDebug>
#include <QProcessEnvironment>

QVAIRenameDialog::QVAIRenameDialog(QWidget *parent, QFileInfo fileInfo) : QDialog(parent), fileInfo(fileInfo)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    setWindowTitle(tr("AI Rename"));
    setMinimumWidth(400);
    
    // Create UI elements
    fileNameEdit = new QLineEdit(this);
    fileNameEdit->setText(fileInfo.fileName());
    
    descriptionEdit = new QTextEdit(this);
    descriptionEdit->setPlaceholderText(tr("Additional context for the AI (optional)"));
    descriptionEdit->setMaximumHeight(100);
    
    generateButton = new QPushButton(tr("&Generate AI Name"), this);
    renameButton = new QPushButton(tr("&Rename"), this);
    cancelButton = new QPushButton(tr("Cancel"), this);
    settingsButton = new QPushButton(tr("Settings"), this);
    settingsButton->setToolTip(tr("Configure AI API settings"));
    
    statusLabel = new QLabel(this);
    statusLabel->setText(tr("Enter a description and click 'Generate AI Name'"));
    
    progressBar = new QProgressBar(this);
    progressBar->setVisible(false);
    progressBar->setMaximum(0); // Indeterminate progress
    
    // Layout
    auto *mainLayout = new QVBoxLayout(this);
    
    mainLayout->addWidget(new QLabel(tr("File name:"), this));
    mainLayout->addWidget(fileNameEdit);
    
    mainLayout->addWidget(new QLabel(tr("Additional context (optional):"), this));
    mainLayout->addWidget(descriptionEdit);
    
    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(generateButton);
    buttonLayout->addWidget(settingsButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(renameButton);
    buttonLayout->addWidget(cancelButton);
    
    mainLayout->addWidget(statusLabel);
    mainLayout->addWidget(progressBar);
    mainLayout->addLayout(buttonLayout);
    
    // Connect signals
    connect(generateButton, &QPushButton::clicked, this, &QVAIRenameDialog::onGenerateClicked);
    connect(renameButton, &QPushButton::clicked, this, &QVAIRenameDialog::onRenameClicked);
    connect(cancelButton, &QPushButton::clicked, this, &QVAIRenameDialog::onCancelClicked);
    connect(settingsButton, &QPushButton::clicked, this, &QVAIRenameDialog::loadSettings);
    
    // Network manager for API calls
    networkManager = new QNetworkAccessManager(this);
    connect(networkManager, &QNetworkAccessManager::finished, this, &QVAIRenameDialog::onNetworkReply);
    
    // Initially disable rename button until we have an AI suggestion
    renameButton->setEnabled(false);
}

void QVAIRenameDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    // Select text before extension when dialog is shown
    const auto &lastDot = fileNameEdit->text().lastIndexOf(".");
    if (lastDot != -1)
        fileNameEdit->setSelection(0, lastDot);
}

void QVAIRenameDialog::onGenerateClicked()
{
    generateAIName();
}

void QVAIRenameDialog::onRenameClicked()
{
    performRename();
}

void QVAIRenameDialog::onCancelClicked()
{
    reject();
}

void QVAIRenameDialog::generateAIName()
{
    // Show progress indication
    progressBar->setVisible(true);
    statusLabel->setText(tr("Analyzing image with AI..."));
    generateButton->setEnabled(false);
    
    // Get API configuration - check environment variables first, then fall back to settings
    QString apiKey = qEnvironmentVariable("QVIEW_AI_API_KEY", "");
    if (apiKey.isEmpty()) {
        apiKey = qvApp->getSettingsManager().getString("airename/apikey");
    }
    
    QString baseUrl = qEnvironmentVariable("QVIEW_AI_BASE_URL",
        qvApp->getSettingsManager().getString("airename/baseurl", false).isEmpty() ? 
        "https://api.openai.com/v1" : 
        qvApp->getSettingsManager().getString("airename/baseurl", false));
    QString modelName = qEnvironmentVariable("QVIEW_AI_MODEL",
        qvApp->getSettingsManager().getString("airename/model", false).isEmpty() ? 
        "gpt-4-vision-preview" : 
        qvApp->getSettingsManager().getString("airename/model", false));
    
    // Debug information
    qDebug() << "=== AI Rename Debug Info ===";
    qDebug() << "API Key:" << (apiKey.isEmpty() ? "Empty" : "Set (length:" + QString::number(apiKey.length()) + ")");
    qDebug() << "Base URL:" << baseUrl;
    qDebug() << "Model:" << modelName;
    qDebug() << "File Path:" << fileInfo.absoluteFilePath();
    
    // Check if we're using environment variables
    if (!qEnvironmentVariable("QVIEW_AI_API_KEY", "").isEmpty()) {
        qDebug() << "Using API key from environment variable QVIEW_AI_API_KEY";
    }
    if (!qEnvironmentVariable("QVIEW_AI_BASE_URL", "").isEmpty()) {
        qDebug() << "Using base URL from environment variable QVIEW_AI_BASE_URL";
    }
    if (!qEnvironmentVariable("QVIEW_AI_MODEL", "").isEmpty()) {
        qDebug() << "Using model from environment variable QVIEW_AI_MODEL";
    }
    
    if (apiKey.isEmpty()) {
        statusLabel->setText(tr("API key not configured. Please set it in Options."));
        qDebug() << "Error: API key is empty";
        progressBar->setVisible(false);
        generateButton->setEnabled(true);
        return;
    }
    
    // Read image file data
    QFile imageFile(fileInfo.absoluteFilePath());
    if (!imageFile.open(QIODevice::ReadOnly)) {
        statusLabel->setText(tr("Error: Could not read image file"));
        progressBar->setVisible(false);
        generateButton->setEnabled(true);
        return;
    }
    
    QByteArray imageData = imageFile.readAll();
    imageFile.close();
    
    // Convert image data to base64
    QByteArray base64ImageData = imageData.toBase64();
    
    // Prepare request to AI service
    QString endpoint = baseUrl;
    if (!endpoint.endsWith("/chat/completions")) {
        if (!endpoint.endsWith("/")) {
            endpoint += "/";
        }
        endpoint += "chat/completions";
    }
    
    QNetworkRequest request;
    request.setUrl(QUrl(endpoint));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(apiKey).toUtf8());
    
    // Add User-Agent header
    request.setRawHeader("User-Agent", "qView-AIRename/1.0");
    
    // Prepare JSON payload for vision model
    QJsonObject payload;
    payload["model"] = modelName;
    payload["max_tokens"] = 300;
    
    QJsonArray messages;
    QJsonObject message;
    message["role"] = "user";
    
    QJsonArray contentArray;
    
    // Add text content
    QJsonObject textContent;
    textContent["type"] = "text";
    QString prompt = QString("Generate a descriptive filename (no paths, just filename with extension) for this image originally named '%1'.")
                           .arg(fileInfo.fileName());
    
    if (!descriptionEdit->toPlainText().isEmpty()) {
        prompt += QString(" Additional context: %1").arg(descriptionEdit->toPlainText());
    }
    
    prompt += " Respond with only the suggested filename including appropriate extension.";
    textContent["text"] = prompt;
    contentArray.append(textContent);
    
    // Add image content
    QJsonObject imageContent;
    imageContent["type"] = "image_url";
    QJsonObject imageUrlObj;
    imageUrlObj["url"] = QString("data:image/jpeg;base64,%1").arg(QString::fromLatin1(base64ImageData));
    imageContent["image_url"] = imageUrlObj;
    contentArray.append(imageContent);
    
    message["content"] = contentArray;
    messages.append(message);
    
    payload["messages"] = messages;
    
    QJsonDocument doc(payload);
    QByteArray jsonData = doc.toJson();
    
    // Debug information
    qDebug() << "Request URL:" << endpoint;
    qDebug() << "Request Headers:";
    QList<QByteArray> headerList = request.rawHeaderList();
    for (const QByteArray &header : headerList) {
        qDebug() << "  " << header << ":" << request.rawHeader(header);
    }
    qDebug() << "Request Payload:" << jsonData;
    
    // Send request
    networkManager->post(request, jsonData);
}

void QVAIRenameDialog::performRename()
{
    if (!fileInfo.isWritable()) {
        QMessageBox::critical(this, tr("Error"),
                             tr("Could not rename %1:\nNo write permission or file is read-only.")
                                     .arg(fileInfo.fileName()));
        return;
    }

    const auto newFileName = fileNameEdit->text();
    const auto newFilePath =
            QDir::cleanPath(fileInfo.absolutePath() + QDir::separator() + newFileName);

    emit readyToRenameFile();

    if (fileInfo.absoluteFilePath() != newFilePath) {
        if (QFile::rename(fileInfo.absoluteFilePath(), newFilePath)) {
            emit newFileToOpen(newFilePath);
            accept();
        } else {
            QMessageBox::critical(
                    this, tr("Error"),
                    tr("Could not rename %1:\n(Check that all characters are valid)")
                            .arg(fileInfo.fileName()));
        }
    } else {
        // Filename unchanged
        accept();
    }
}

void QVAIRenameDialog::onNetworkReply(QNetworkReply *reply)
{
    progressBar->setVisible(false);
    generateButton->setEnabled(true);
    
    // Debug information
    qDebug() << "=== AI Response Debug Info ===";
    qDebug() << "HTTP Status Code:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    qDebug() << "Error:" << reply->error();
    qDebug() << "Error String:" << reply->errorString();
    
    if (reply->error() != QNetworkReply::NoError) {
        statusLabel->setText(tr("Error: %1").arg(reply->errorString()));
        qDebug() << "Network Error:" << reply->errorString();
        reply->deleteLater();
        return;
    }
    
    QByteArray response_data = reply->readAll();
    reply->deleteLater();
    
    // Debug information
    qDebug() << "Response Data:" << response_data;
    
    QJsonDocument jsonResponse = QJsonDocument::fromJson(response_data);
    qDebug() << "JSON Parse Result - Null:" << jsonResponse.isNull() << "Is Object:" << jsonResponse.isObject();
    if (jsonResponse.isNull() || !jsonResponse.isObject()) {
        statusLabel->setText(tr("Error parsing AI response"));
        qDebug() << "Error: Failed to parse JSON response";
        return;
    }
    
    QJsonObject responseObject = jsonResponse.object();
    qDebug() << "Response Object Keys:" << responseObject.keys();
    if (responseObject.contains("choices") && responseObject["choices"].isArray()) {
        QJsonArray choices = responseObject["choices"].toArray();
        qDebug() << "Choices Array Size:" << choices.size();
        if (!choices.isEmpty() && choices.first().isObject()) {
            QJsonObject choice = choices.first().toObject();
            qDebug() << "Choice Object Keys:" << choice.keys();
            if (choice.contains("message") && choice["message"].isObject()) {
                QJsonObject message = choice["message"].toObject();
                qDebug() << "Message Object Keys:" << message.keys();
                if (message.contains("content") && message["content"].isString()) {
                    QString aiSuggestion = message["content"].toString().trimmed();
                    qDebug() << "AI Suggestion:" << aiSuggestion;
                    // Clean up the suggestion - remove any surrounding quotes
                    if (aiSuggestion.startsWith("\"") && aiSuggestion.endsWith("\"") && aiSuggestion.length() > 2) {
                        aiSuggestion = aiSuggestion.mid(1, aiSuggestion.length() - 2);
                    }
                    
                    // Validate and clean the filename to ensure it's filesystem safe
                    QString cleanedName = aiSuggestion;
                    // Remove any path components
                    if (cleanedName.contains('/')) {
                        cleanedName = cleanedName.split('/').last();
                    }
                    if (cleanedName.contains('\\')) {
                        cleanedName = cleanedName.split('\\').last();
                    }
                    
                    // Remove invalid characters for most filesystems
                    cleanedName = cleanedName.remove(QRegularExpression("[<>:\"/\\|?*\x00-\x1F]"));
                    
                    // Ensure the filename isn't empty
                    if (cleanedName.isEmpty()) {
                        cleanedName = fileInfo.completeBaseName();
                    }
                    
                    // Ensure there's an extension
                    if (!cleanedName.contains('.')) {
                        QString originalExt = fileInfo.suffix();
                        if (!originalExt.isEmpty()) {
                            cleanedName += "." + originalExt;
                        }
                    }
                    
                    suggestedName = cleanedName;
                    fileNameEdit->setText(cleanedName);
                    statusLabel->setText(tr("AI suggested name: %1").arg(cleanedName));
                    renameButton->setEnabled(true);
                    return;
                }
            }
        }
    }
    
    statusLabel->setText(tr("Unexpected AI response format"));
}

void QVAIRenameDialog::loadSettings()
{
    // For now, we'll just show a message about where to configure settings
    // In a full implementation, this would open a settings dialog
    QMessageBox::information(this, tr("AI Settings"), 
        tr("AI settings can be configured in the main application options.\n"
           "Go to Tools → Settings → AI Rename to configure your API key,\n"
           "base URL, and model name."));
}

void QVAIRenameDialog::saveSettings()
{
    // This would save settings in a full implementation
    // For now, settings are handled by the main application
}