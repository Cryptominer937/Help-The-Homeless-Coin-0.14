// Copyright (c) 2011-2015 The Bitcoin Core developers
// Copyright (c) 2014-2018 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "overviewapage.h"
#include "ui_overviewapage.h"


#include "bitcoinunits.h"
#include "clientmodel.h"
#include "guiconstants.h"
#include "guiutil.h"
#include "init.h"
#include "optionsmodel.h"
#include "platformstyle.h"
/* #include "transactionfilterproxy.h" 
#include "transactiontablemodel.h" */
#include "utilitydialog.h"
#include "walletmodel.h"


#include "instantx.h"
#include "masternode-sync.h"
#include "privatesend-client.h"

#include <QAbstractItemDelegate>
#include <QPainter>
#include <QSettings>
#include <QTimer>
#include <QUrl>
#include <QDesktopServices>

#define ICON_OFFSET 16
#define DECORATION_SIZE 54
#define NUM_ITEMS 5
#define NUM_ITEMS_ADV 7


/* class TxViewDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    TxViewDelegate(const PlatformStyle *_platformStyle, QObject *parent=nullptr):
        QAbstractItemDelegate(), unit(BitcoinUnits::HTH),
        platformStyle(_platformStyle)
    {
    }
    inline void paint(QPainter *painter, const QStyleOptionViewItem &option,
                      const QModelIndex &index ) const
    {
        painter->save();
        QIcon icon = qvariant_cast<QIcon>(index.data(TransactionTableModel::RawDecorationRole));
        QRect mainRect = option.rect;
        mainRect.moveLeft(ICON_OFFSET);
        QRect decorationRect(mainRect.topLeft(), QSize(DECORATION_SIZE, DECORATION_SIZE));
        int xspace = DECORATION_SIZE + 8;
        int ypad = 6;
        int halfheight = (mainRect.height() - 2*ypad)/2;
        QRect amountRect(mainRect.left() + xspace, mainRect.top()+ypad, mainRect.width() - xspace - ICON_OFFSET, halfheight);
        QRect addressRect(mainRect.left() + xspace, mainRect.top()+ypad+halfheight, mainRect.width() - xspace, halfheight);
        icon = platformStyle->SingleColorIcon(icon);
        icon.paint(painter, decorationRect);
        QDateTime date = index.data(TransactionTableModel::DateRole).toDateTime();
        QString address = index.data(Qt::DisplayRole).toString();
        qint64 amount = index.data(TransactionTableModel::AmountRole).toLongLong();
        bool confirmed = index.data(TransactionTableModel::ConfirmedRole).toBool();
        QVariant value = index.data(Qt::ForegroundRole);
        QColor foreground = option.palette.color(QPalette::Text);
        if(value.canConvert<QBrush>())
        {
            QBrush brush = qvariant_cast<QBrush>(value);
            foreground = brush.color();
        }
        painter->setPen(foreground);
        QRect boundingRect;
        painter->drawText(addressRect, Qt::AlignLeft|Qt::AlignVCenter, address, &boundingRect);
        if (index.data(TransactionTableModel::WatchonlyRole).toBool())
        {
            QIcon iconWatchonly = qvariant_cast<QIcon>(index.data(TransactionTableModel::WatchonlyDecorationRole));
            QRect watchonlyRect(boundingRect.right() + 5, mainRect.top()+ypad+halfheight, 16, halfheight);
            iconWatchonly.paint(painter, watchonlyRect);
        }
        if(amount < 0)
        {
            foreground = COLOR_NEGATIVE;
        }
        else if(!confirmed)
        {
            foreground = COLOR_UNCONFIRMED;
        }
        else
        {
            foreground = option.palette.color(QPalette::Text);
        }
        painter->setPen(foreground);
        QString amountText = BitcoinUnits::floorWithUnit(unit, amount, true, BitcoinUnits::separatorAlways);
        if(!confirmed)
        {
            amountText = QString("[") + amountText + QString("]");
        }
        painter->drawText(amountRect, Qt::AlignRight|Qt::AlignVCenter, amountText);
        painter->setPen(option.palette.color(QPalette::Text));
        painter->drawText(amountRect, Qt::AlignLeft|Qt::AlignVCenter, GUIUtil::dateTimeStr(date));
        painter->restore();
    }
    inline QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        return QSize(DECORATION_SIZE, DECORATION_SIZE);
    }
    int unit;
    const PlatformStyle *platformStyle;
}; */

 #include "overviewapage.moc"



OverviewAPage::OverviewAPage(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    timer(nullptr),
    ui(new Ui::OverviewAPage),
    clientModel(0),
    walletModel(0),
/*    currentBalance(-1),
    currentUnconfirmedBalance(-1),
    currentImmatureBalance(-1),
    currentWatchOnlyBalance(-1),
    currentWatchUnconfBalance(-1),
    currentWatchImmatureBalance(-1), */
    cachedNumISLocks(-1)
 /*   txdelegate(new TxViewDelegate(platformStyle, this)) */
{
    nDisplayUnit = 0; // just make sure it's not unitialized   
    ui->setupUi(this);
    QString theme = GUIUtil::getThemeName();

          
     /* ui->pushButton_Website->setIcon(QIcon(GUIUtil::getThemeImage(":/icons/website"))); */
   /*  ui->pushButton_Website->setStatusTip(tr("Visit Help The Homeless Worldwide A NJ Nonprofit Corporation"));
    ui->pushButton_Website_1->setStatusTip(tr("Visit Help The Homeless Coin")); */
        
    // Recent transactions
  /*  ui->listTransactions->setItemDelegate(txdelegate);
    ui->listTransactions->setIconSize(QSize(DECORATION_SIZE, DECORATION_SIZE));
    // Note: minimum height of listTransactions will be set later in updateAdvancedPSUI() to reflect actual settings
    ui->listTransactions->setAttribute(Qt::WA_MacShowFocusRect, false); 
    connect(ui->listTransactions, SIGNAL(clicked(QModelIndex)), this, SLOT(handleTransactionClicked(QModelIndex)));
    // init "out of sync" warning labels
    ui->labelWalletStatus->setText("(" + tr("out of sync") + ")");  */
    ui->labelPrivateSendEnabled_2->setText("(" + tr("out of sync") + ")");
 /*   ui->labelTransactionsStatus->setText("(" + tr("out of sync") + ")");  */

    // hide PS frame (helps to preserve saved size)
    // we'll setup and make it visible in updateAdvancedPSUI() later if we are not in litemode
    ui->togglePrivateSend_2->setVisible(false);
       
        
    // start with displaying the "out of sync" warnings
    showOutOfSyncWarning(true);

    // that's it for litemode
    if(fLiteMode) return;

    // Disable any PS UI for masternode or when autobackup is disabled or failed for whatever reason
    if(fMasternodeMode || nWalletBackups <= 0){
        DisablePrivateSendCompletely();
        if (nWalletBackups <= 0) {
            ui->labelPrivateSendEnabled_2->setToolTip(tr("Automatic backups are disabled, no mixing available!"));
        }
    } else {
        if(!privateSendClient.fEnablePrivateSend){
            ui->togglePrivateSend_2->setText(tr("Start Mixing"));
        } else {
            ui->togglePrivateSend_2->setText(tr("Stop Mixing"));
        }
        // Disable privateSendClient builtin support for automatic backups while we are in GUI,
        // we'll handle automatic backups and user warnings in privateSendStatus()
        privateSendClient.fCreateAutoBackups = false;
        timer = new QTimer(this);
        connect(timer, SIGNAL(timeout()), this, SLOT(privateSendStatus()));
        timer->start(1000);
    }
}

/* void OverviewAPage::handleTransactionClicked(const QModelIndex &index)
{
    if(filter)
        Q_EMIT transactionClicked(filter->mapToSource(index));
}
*/

 void OverviewAPage::handleOutOfSyncWarningClicks()
{
    Q_EMIT outOfSyncWarningClicked();
}


 OverviewAPage::~OverviewAPage()
{
    if(timer) disconnect(timer, SIGNAL(timeout()), this, SLOT(privateSendStatus()));
    delete ui;
}

void OverviewAPage::setBalance(const CAmount& balance, const CAmount& unconfirmedBalance, const CAmount& immatureBalance, const CAmount& anonymizedBalance, const CAmount& watchOnlyBalance, const CAmount& watchUnconfBalance, const CAmount& watchImmatureBalance)
{
    currentBalance = balance;
    currentUnconfirmedBalance = unconfirmedBalance;
    currentImmatureBalance = immatureBalance;
    currentAnonymizedBalance = anonymizedBalance;
    currentWatchOnlyBalance = watchOnlyBalance;
    currentWatchUnconfBalance = watchUnconfBalance;
    currentWatchImmatureBalance = watchImmatureBalance;
/*    ui->labelBalance->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, balance, false, BitcoinUnits::separatorAlways));
    ui->labelUnconfirmed->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, unconfirmedBalance, false, BitcoinUnits::separatorAlways));
    ui->labelImmatureText->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, immatureBalance, false, BitcoinUnits::separatorAlways)); */
    ui->labelAnonymized->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, anonymizedBalance, false, BitcoinUnits::separatorAlways));
 /*   ui->labelTotal->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, balance + unconfirmedBalance + immatureBalance, false, BitcoinUnits::separatorAlways));
    ui->labelWatchImmature->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, watchOnlyBalance, false, BitcoinUnits::separatorAlways));
    ui->labelWatchImmature->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, watchUnconfBalance, false, BitcoinUnits::separatorAlways));
    ui->labelWatchImmature->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, watchImmatureBalance, false, BitcoinUnits::separatorAlways));
    ui->labelTotal->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, watchOnlyBalance + watchUnconfBalance + watchImmatureBalance, false, BitcoinUnits::separatorAlways));
    // only show immature (newly mined) balance if it's non-zero, so as not to complicate things
    // for the non-mining users
    bool showImmature = immatureBalance != 0;
    bool showWatchOnlyImmature = watchImmatureBalance != 0;
    // for symmetry reasons also show immature label when the watch-only one is shown
    ui->labelImmatureText->setVisible(showImmature || showWatchOnlyImmature);
    ui->labelImmatureText->setVisible(showImmature || showWatchOnlyImmature);
    ui->labelWatchImmature->setVisible(showWatchOnlyImmature); // show watch-only immature balance */

   updatePrivateSendProgress();
    if (walletModel) {
        int numISLocks = walletModel->getNumISLocks();
        if(cachedNumISLocks != numISLocks) {
            cachedNumISLocks = numISLocks;
       /*     ui->listTransactions->update();  */
        }
    } 
}

// show/hide watch-only labels
/*void OverviewAPage::updateWatchOnlyLabels(bool showWatchOnly)
{
   ui->labelTotal->setVisible(showWatchOnly);      // show spendable label (only when watch-only is active)
    ui->labelWatchTotal->setVisible(showWatchOnly);      // show watch-only label
    ui->lineWatchBalance->setVisible(showWatchOnly);    // show watch-only balance separator line  
    ui->labelWatchAvailable->setVisible(showWatchOnly); // show watch-only available balance
    ui->labelWatchPending->setVisible(showWatchOnly);   // show watch-only pending balance
    ui->labelWatchTotal->setVisible(showWatchOnly);     // show watch-only total balance

    if (!showWatchOnly){
        ui->labelWatchImmature->hide();
    }
    else{
        ui->labelBalanceText->setIndent(20);
        ui->labelWatchPending->setIndent(20);
        ui->labelImmatureText->setIndent(20);
        ui->labelTotal->setIndent(20);
    }
} */


void OverviewAPage::setClientModel(ClientModel *model)
{
    this->clientModel = model;
    if(model)
    {
        // Show warning if this is a prerelease version
     /*   connect(model, SIGNAL(alertsChanged(QString)), this, SLOT(updateAlerts(QString)));
         updateAlerts(model->getStatusBarWarnings()); */
    }
} 

void OverviewAPage::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
    if(model && model->getOptionsModel())
    {
        // update the display unit, to not use the default ("HTH")
        updateDisplayUnit();
        // Keep up to date with wallet
        setBalance(model->getBalance(), model->getUnconfirmedBalance(), model->getImmatureBalance(),  model->getAnonymizedBalance(),
                   model->getWatchBalance(), model->getWatchUnconfirmedBalance(), model->getWatchImmatureBalance());
        connect(model, SIGNAL(balanceChanged(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount)), this, SLOT(setBalance(CAmount, CAmount,CAmount,CAmount,CAmount,CAmount,CAmount)));
        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));
      /*  updateWatchOnlyLabels(model->haveWatchOnly()); 
        connect(model, SIGNAL(notifyWatchonlyChanged(bool)), this, SLOT(updateWatchOnlyLabels(bool))); */

        // explicitly update PS frame and transaction list to reflect actual settings
        updateAdvancedPSUI(model->getOptionsModel()->getShowAdvancedPSUI());
        // that's it for litemode
        if(fLiteMode) return;
        connect(model->getOptionsModel(), SIGNAL(privateSendRoundsChanged()), this, SLOT(updatePrivateSendProgress()));
        connect(model->getOptionsModel(), SIGNAL(privateSentAmountChanged()), this, SLOT(updatePrivateSendProgress()));
        connect(model->getOptionsModel(), SIGNAL(advancedPSUIChanged(bool)), this, SLOT(updateAdvancedPSUI(bool)));
        connect(ui->privateSendAuto_2, SIGNAL(clicked()), this, SLOT(privateSendAuto_2()));
        connect(ui->privateSendReset_2, SIGNAL(clicked()), this, SLOT(privateSendReset_2()));
        connect(ui->privateSendInfo_2, SIGNAL(clicked()), this, SLOT(privateSendInfo_2()));
        
        connect(ui->togglePrivateSend_2, SIGNAL(clicked()), this, SLOT(togglePrivateSend_2()));
        
        // privatesend buttons will not react to spacebar must be clicked on
        ui->privateSendAuto_2->setFocusPolicy(Qt::NoFocus);
        ui->privateSendReset_2->setFocusPolicy(Qt::NoFocus);
        ui->privateSendInfo_2->setFocusPolicy(Qt::NoFocus);
        ui->togglePrivateSend_2->setFocusPolicy(Qt::NoFocus);  
     
    }
}



 void OverviewAPage::updateDisplayUnit()
{
    if(walletModel && walletModel->getOptionsModel())
    {
        nDisplayUnit = walletModel->getOptionsModel()->getDisplayUnit();
        if(currentBalance != -1)
            setBalance(currentBalance, currentUnconfirmedBalance, currentImmatureBalance, currentAnonymizedBalance,
                       currentWatchOnlyBalance, currentWatchUnconfBalance, currentWatchImmatureBalance);

        // Update txdelegate->unit with the current unit
       /* txdelegate->unit = nDisplayUnit; 
        ui->listTransactions->update(); */
    }
}


 /* void OverviewAPage::updateAlerts(const QString &warnings)
{
   this->ui->labelAlerts->setVisible(!warnings.isEmpty());
    this->ui->labelAlerts->setText(warnings);
}   */



void OverviewAPage::showOutOfSyncWarning(bool fShow)
{
    ui->listTransactions_2->setVisible(fShow);  
    ui->labelPrivateSendEnabled_2->setVisible(fShow);   
    ui->listTransactions_2->setVisible(fShow);  
}

void OverviewAPage::updatePrivateSendProgress()
{
    if(!masternodeSync.IsBlockchainSynced() || ShutdownRequested()) return;
    if(!pwalletMain) return;
    QString strAmountAndRounds;
    QString strPrivateSendAmount = BitcoinUnits::formatHtmlWithUnit(nDisplayUnit, privateSendClient.nPrivateSendAmount * COIN, false, BitcoinUnits::separatorAlways);
    if(currentBalance == 0)
    {
        ui->privateSendProgress_2->setValue(0);
        ui->privateSendProgress_2->setToolTip(tr("No inputs detected"));
        // when balance is zero just show info from settings
        strPrivateSendAmount = strPrivateSendAmount.remove(strPrivateSendAmount.indexOf("."), BitcoinUnits::decimals(nDisplayUnit) + 1);
        strAmountAndRounds = strPrivateSendAmount + " / " + tr("%n Rounds", "", privateSendClient.nPrivateSendRounds);
        ui->labelAmountRounds_2->setToolTip(tr("No inputs detected"));
        ui->labelAmountRounds_2->setText(strAmountAndRounds);
        return;
    }
    CAmount nAnonymizableBalance = pwalletMain->GetAnonymizableBalance(false, false);
    CAmount nMaxToAnonymize = nAnonymizableBalance + currentAnonymizedBalance;
    // If it's more than the anon threshold, limit to that.
    if(nMaxToAnonymize > privateSendClient.nPrivateSendAmount*COIN) nMaxToAnonymize = privateSendClient.nPrivateSendAmount*COIN;
    if(nMaxToAnonymize == 0) return;
    if(nMaxToAnonymize >= privateSendClient.nPrivateSendAmount * COIN) {
        ui->labelAmountRounds_2->setToolTip(tr("Found enough compatible inputs to anonymize %1")
                                          .arg(strPrivateSendAmount));
        strPrivateSendAmount = strPrivateSendAmount.remove(strPrivateSendAmount.indexOf("."), BitcoinUnits::decimals(nDisplayUnit) + 1);
        strAmountAndRounds = strPrivateSendAmount + " / " + tr("%n Rounds", "", privateSendClient.nPrivateSendRounds);
    } else {
        QString strMaxToAnonymize = BitcoinUnits::formatHtmlWithUnit(nDisplayUnit, nMaxToAnonymize, false, BitcoinUnits::separatorAlways);
        ui->labelAmountRounds_2->setToolTip(tr("Not enough compatible inputs to anonymize <span style='color:red;'>%1</span>,<br>"
                                             "will anonymize <span style='color:red;'>%2</span> instead")
                                          .arg(strPrivateSendAmount)
                                          .arg(strMaxToAnonymize));
        strMaxToAnonymize = strMaxToAnonymize.remove(strMaxToAnonymize.indexOf("."), BitcoinUnits::decimals(nDisplayUnit) + 1);
        strAmountAndRounds = "<span style='color:red;'>" +
                QString(BitcoinUnits::factor(nDisplayUnit) == 1 ? "" : "~") + strMaxToAnonymize +
                " / " + tr("%n Rounds", "", privateSendClient.nPrivateSendRounds) + "</span>";
    }
    ui->labelAmountRounds_2->setText(strAmountAndRounds);
    
    if (!fShowAdvancedPSUI) return;

    CAmount nDenominatedConfirmedBalance;
    CAmount nDenominatedUnconfirmedBalance;
    CAmount nNormalizedAnonymizedBalance;
    float nAverageAnonymizedRounds;

    nDenominatedConfirmedBalance = pwalletMain->GetDenominatedBalance();
    nDenominatedUnconfirmedBalance = pwalletMain->GetDenominatedBalance(true);
    nNormalizedAnonymizedBalance = pwalletMain->GetNormalizedAnonymizedBalance();
    nAverageAnonymizedRounds = pwalletMain->GetAverageAnonymizedRounds();

    // calculate parts of the progress, each of them shouldn't be higher than 1
    // progress of denominating
    float denomPart = 0;
    // mixing progress of denominated balance
    float anonNormPart = 0;
    // completeness of full amount anonymization
    float anonFullPart = 0;

    CAmount denominatedBalance = nDenominatedConfirmedBalance + nDenominatedUnconfirmedBalance;
    denomPart = (float)denominatedBalance / nMaxToAnonymize;
    denomPart = denomPart > 1 ? 1 : denomPart;
    denomPart *= 100;

    anonNormPart = (float)nNormalizedAnonymizedBalance / nMaxToAnonymize;
    anonNormPart = anonNormPart > 1 ? 1 : anonNormPart;
    anonNormPart *= 100;

    anonFullPart = (float)currentAnonymizedBalance / nMaxToAnonymize;
    anonFullPart = anonFullPart > 1 ? 1 : anonFullPart;
    anonFullPart *= 100;

    // apply some weights to them ...
    float denomWeight = 1;
    float anonNormWeight = privateSendClient.nPrivateSendRounds;
    float anonFullWeight = 2;
    float fullWeight = denomWeight + anonNormWeight + anonFullWeight;
    // ... and calculate the whole progress
    float denomPartCalc = ceilf((denomPart * denomWeight / fullWeight) * 100) / 100;
    float anonNormPartCalc = ceilf((anonNormPart * anonNormWeight / fullWeight) * 100) / 100;
    float anonFullPartCalc = ceilf((anonFullPart * anonFullWeight / fullWeight) * 100) / 100;
    float progress = denomPartCalc + anonNormPartCalc + anonFullPartCalc;
    if(progress >= 100) progress = 100;

    ui->privateSendProgress_2->setValue(progress);

    QString strToolPip = ("<b>" + tr("Overall progress") + ": %1%</b><br/>" +
                          tr("Denominated") + ": %2%<br/>" +
                          tr("Mixed") + ": %3%<br/>" +
                          tr("Anonymized") + ": %4%<br/>" +
                          tr("Denominated inputs have %5 of %n rounds on average", "", privateSendClient.nPrivateSendRounds))
            .arg(progress).arg(denomPart).arg(anonNormPart).arg(anonFullPart)
            .arg(nAverageAnonymizedRounds);
    ui->privateSendProgress_2->setToolTip(strToolPip);
}

void OverviewAPage::updateAdvancedPSUI(bool fShowAdvancedPSUI) {
    this->fShowAdvancedPSUI = fShowAdvancedPSUI;
    int nNumItems = (fLiteMode || !fShowAdvancedPSUI) ? NUM_ITEMS : NUM_ITEMS_ADV;
  /*  SetupTransactionList(nNumItems); */

    if (fLiteMode) return;

    ui->togglePrivateSend_2->setVisible(true);
    ui->labelCompletitionText_2->setVisible(fShowAdvancedPSUI);
    ui->privateSendProgress_2->setVisible(fShowAdvancedPSUI);
    ui->labelSubmittedDenomText_2->setVisible(fShowAdvancedPSUI);
    ui->labelSubmittedDenom_2->setVisible(fShowAdvancedPSUI);
    ui->privateSendAuto_2->setVisible(fShowAdvancedPSUI);
    ui->privateSendReset_2->setVisible(fShowAdvancedPSUI);
    ui->privateSendInfo_2->setVisible(true);
    ui->labelPrivateSendLastMessage_2->setVisible(fShowAdvancedPSUI);
    
}


void OverviewAPage::privateSendStatus()
{
    if(!masternodeSync.IsBlockchainSynced() || ShutdownRequested()) return;
    static int64_t nLastDSProgressBlockTime = 0;
    int nBestHeight = clientModel->getNumBlocks();
    // We are processing more then 1 block per second, we'll just leave
    if(((nBestHeight - privateSendClient.nCachedNumBlocks) / (GetTimeMillis() - nLastDSProgressBlockTime + 1) > 1)) return;
    nLastDSProgressBlockTime = GetTimeMillis();
    QString strKeysLeftText(tr("keys left: %1").arg(pwalletMain->nKeysLeftSinceAutoBackup));
    if(pwalletMain->nKeysLeftSinceAutoBackup < PRIVATESEND_KEYS_THRESHOLD_WARNING) {
        strKeysLeftText = "<span style='color:red;'>" + strKeysLeftText + "</span>";
    }
    ui->labelPrivateSendEnabled_2->setToolTip(strKeysLeftText);
    if (!privateSendClient.fEnablePrivateSend) {
        if (nBestHeight != privateSendClient.nCachedNumBlocks) {
            privateSendClient.nCachedNumBlocks = nBestHeight;
            updatePrivateSendProgress();
        }
        ui->labelPrivateSendLastMessage_2->setText("");
        ui->togglePrivateSend_2->setText(tr("Start Mixing"));
        QString strEnabled = tr("Disabled");
        // Show how many keys left in advanced PS UI mode only
        if (fShowAdvancedPSUI) strEnabled += ", " + strKeysLeftText;
        ui->labelPrivateSendEnabled_2->setText(strEnabled);
        return;
    }
    // Warn user that wallet is running out of keys
    // NOTE: we do NOT warn user and do NOT create autobackups if mixing is not running
    if (nWalletBackups > 0 && pwalletMain->nKeysLeftSinceAutoBackup < PRIVATESEND_KEYS_THRESHOLD_WARNING) {
        QSettings settings;
        if(settings.value("fLowKeysWarning").toBool()) {
            QString strWarn =   tr("Very low number of keys left since last automatic backup!") + "<br><br>" +
                                tr("We are about to create a new automatic backup for you, however "
                                   "<span style='color:red;'> you should always make sure you have backups "
                                   "saved in some safe place</span>!") + "<br><br>" +
                                tr("Note: You can turn this message off in options.");
            ui->labelPrivateSendEnabled_2->setToolTip(strWarn);
            LogPrintf("OverviewAPage::privateSendStatus -- Very low number of keys left since last automatic backup, warning user and trying to create new backup...\n");
            QMessageBox::warning(this, tr("PrivateSend"), strWarn, QMessageBox::Ok, QMessageBox::Ok);
        } else {
            LogPrintf("OverviewAPage::privateSendStatus -- Very low number of keys left since last automatic backup, skipping warning and trying to create new backup...\n");
        }
        std::string strBackupWarning;
        std::string strBackupError;
        if(!AutoBackupWallet(pwalletMain, "", strBackupWarning, strBackupError)) {
            if (!strBackupWarning.empty()) {
                // It's still more or less safe to continue but warn user anyway
                LogPrintf("OverviewAPage::privateSendStatus -- WARNING! Something went wrong on automatic backup: %s\n", strBackupWarning);
                QMessageBox::warning(this, tr("PrivateSend"),
                    tr("WARNING! Something went wrong on automatic backup") + ":<br><br>" + strBackupWarning.c_str(),
                    QMessageBox::Ok, QMessageBox::Ok);
            }
            if (!strBackupError.empty()) {
                // Things are really broken, warn user and stop mixing immediately
                LogPrintf("OverviewAPage::privateSendStatus -- ERROR! Failed to create automatic backup: %s\n", strBackupError);
                QMessageBox::warning(this, tr("PrivateSend"),
                    tr("ERROR! Failed to create automatic backup") + ":<br><br>" + strBackupError.c_str() + "<br>" +
                    tr("Mixing is disabled, please close your wallet and fix the issue!"),
                    QMessageBox::Ok, QMessageBox::Ok);
            }
        }
    }
    QString strEnabled = privateSendClient.fEnablePrivateSend ? tr("Enabled") : tr("Disabled");
    // Show how many keys left in advanced PS UI mode only
    if(fShowAdvancedPSUI) strEnabled += ", " + strKeysLeftText;
    ui->labelPrivateSendEnabled_2->setText(strEnabled);
    if(nWalletBackups == -1) {
        // Automatic backup failed, nothing else we can do until user fixes the issue manually
        DisablePrivateSendCompletely();
        QString strError =  tr("ERROR! Failed to create automatic backup") + ", " +
                            tr("see debug.log for details.") + "<br><br>" +
                            tr("Mixing is disabled, please close your wallet and fix the issue!");
        ui->labelPrivateSendEnabled_2->setToolTip(strError);
        return;
    } else if(nWalletBackups == -2) {
        // We were able to create automatic backup but keypool was not replenished because wallet is locked.
        QString strWarning = tr("WARNING! Failed to replenish keypool, please unlock your wallet to do so.");
        ui->labelPrivateSendEnabled_2->setToolTip(strWarning);
    }
    // check privatesend status and unlock if needed
    if(nBestHeight != privateSendClient.nCachedNumBlocks) {
        // Balance and number of transactions might have changed
        privateSendClient.nCachedNumBlocks = nBestHeight;
        updatePrivateSendProgress();
    }
    QString strStatus = QString(privateSendClient.GetStatuses().c_str());
    QString s = tr("Last PrivateSend message:\n") + strStatus;
    if(s != ui->labelPrivateSendLastMessage_2->text())
        LogPrint("privatesend", "OverviewAPage::privateSendStatus -- Last PrivateSend message: %s\n", strStatus.toStdString());
    ui->labelPrivateSendLastMessage_2->setText(s);
    ui->labelSubmittedDenom_2->setText(QString(privateSendClient.GetSessionDenoms().c_str()));
}
void OverviewAPage::privateSendAuto_2(){
    privateSendClient.DoAutomaticDenominating(*g_connman);
}
void OverviewAPage::privateSendReset_2(){
    privateSendClient.ResetPool();
    QMessageBox::warning(this, tr("PrivateSend"),
        tr("PrivateSend was successfully reset."),
        QMessageBox::Ok, QMessageBox::Ok);
}
void OverviewAPage::privateSendInfo_2(){
    HelpMessageDialog dlg(this, HelpMessageDialog::pshelp);
    dlg.exec();
}
void OverviewAPage::togglePrivateSend(){
    QSettings settings;
    // Popup some information on first mixing
    QString hasMixed = settings.value("hasMixed").toString();
    if(hasMixed.isEmpty()){
        QMessageBox::information(this, tr("PrivateSend"),
                tr("If you don't want to see internal PrivateSend fees/transactions select \"Most Common\" as Type on the \"Transactions\" tab."),
                QMessageBox::Ok, QMessageBox::Ok);
        settings.setValue("hasMixed", "hasMixed");
    }
    if(!privateSendClient.fEnablePrivateSend){
        const CAmount nMinAmount = CPrivateSend::GetSmallestDenomination() + CPrivateSend::GetMaxCollateralAmount();
        if(currentBalance < nMinAmount){
            QString strMinAmount(BitcoinUnits::formatWithUnit(nDisplayUnit, nMinAmount));
            QMessageBox::warning(this, tr("PrivateSend"),
                tr("PrivateSend requires at least %1 to use.").arg(strMinAmount),
                QMessageBox::Ok, QMessageBox::Ok);
            return;
        }
        // if wallet is locked, ask for a passphrase
        if (walletModel->getEncryptionStatus() == WalletModel::Locked)
        {
            WalletModel::UnlockContext ctx(walletModel->requestUnlock(true));
            if(!ctx.isValid())
            {
                //unlock was cancelled
                privateSendClient.nCachedNumBlocks = std::numeric_limits<int>::max();
                QMessageBox::warning(this, tr("PrivateSend"),
                    tr("Wallet is locked and user declined to unlock. Disabling PrivateSend."),
                    QMessageBox::Ok, QMessageBox::Ok);
                LogPrint("privatesend", "OverviewAPage::togglePrivateSend -- Wallet is locked and user declined to unlock. Disabling PrivateSend.\n");
                return;
            }
        }
    }
    privateSendClient.fEnablePrivateSend = !privateSendClient.fEnablePrivateSend;
    privateSendClient.nCachedNumBlocks = std::numeric_limits<int>::max();
    if(!privateSendClient.fEnablePrivateSend){
        ui->togglePrivateSend_2->setText(tr("Start Mixing"));
        privateSendClient.ResetPool();
    } else {
        ui->togglePrivateSend_2->setText(tr("Stop Mixing"));
    }   
} 

/* void OverviewAPage::SetupTransactionList(int nNumItems) {
    ui->listTransactions->setMinimumHeight(nNumItems * (DECORATION_SIZE + 2));
    if(walletModel && walletModel->getOptionsModel()) {
        // Set up transaction list
        filter.reset(new TransactionFilterProxy());
        filter->setSourceModel(walletModel->getTransactionTableModel());
        filter->setLimit(nNumItems);
        filter->setDynamicSortFilter(true);
        filter->setSortRole(Qt::EditRole);
        filter->setShowInactive(false);
        filter->sort(TransactionTableModel::Date, Qt::DescendingOrder);
        ui->listTransactions->setModel(filter.get());
        ui->listTransactions->setModelColumn(TransactionTableModel::ToAddress);
    }
}
*/

  void OverviewAPage::DisablePrivateSendCompletely() {
    ui->togglePrivateSend_2->setText("(" + tr("Disabled") + ")");
    ui->privateSendAuto_2->setText("(" + tr("Disabled") + ")");
    ui->privateSendReset_2->setText("(" + tr("Disabled") + ")");
    ui->togglePrivateSend_2->setEnabled(false);
    if (nWalletBackups <= 0) {
        ui->labelPrivateSendEnabled_2->setText("<span style='color:red;'>(" + tr("Disabled") + ")</span>");
    }
    privateSendClient.fEnablePrivateSend = false;
}
 
/************** HTH Worldwide Button ******************/
 
void OverviewAPage::on_pushButton_Website_clicked() {  // Nonprofit Wesbite
    
    QDesktopServices::openUrl(QUrl("https://www.helpthehomelessworldwide.org/", QUrl::TolerantMode));
    
}

void OverviewAPage::on_pushButton_Website_1_clicked() {  // HTH Coin Wesbite
    
    QDesktopServices::openUrl(QUrl("https://hth.world", QUrl::TolerantMode));
    
}


void OverviewAPage::on_pushButton_Website_2_clicked() {  // HTH Exchanges
    
    QDesktopServices::openUrl(QUrl("https://hth.world/exchanges.html", QUrl::TolerantMode));
    
}

void OverviewAPage::on_pushButton_Website_3_clicked() {  // HTH Explorer
    
    QDesktopServices::openUrl(QUrl("https://chainz.cryptoid.info/hth", QUrl::TolerantMode));
    
}

void OverviewAPage::on_pushButton_Website_4_clicked() {  // HTH Partners
    
    QDesktopServices::openUrl(QUrl("https://hth.world/partners.html", QUrl::TolerantMode));
    
}

/************** HTH Worldwide Button ******************/
