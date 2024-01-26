#include "BackupsLayer.h"
#include <CreateBackupLayer.h>
#include <BackupCell.h>
#include <Geode/ui/GeodeUI.hpp>

BackupsLayer* BackupsLayer::create() {
    auto ret = new BackupsLayer();
    if (ret && ret->init(470, 260, "GJ_square01.png")) {
        ret->autorelease();
    } else {
        delete ret;
        ret = nullptr;
    }
    return ret;
}

bool BackupsLayer::init(float _w, float _h, const char* _spr){
    //create window
    auto winSize = CCDirector::sharedDirector()->getWinSize();
    CCSize size = {_w, _h};

    if (!this->initWithColor({0, 0, 0, 105})) return false;
    m_mainLayer = CCLayer::create();
    this->addChild(m_mainLayer);

    CCScale9Sprite* bg = CCScale9Sprite::create(_spr, {0.0f, 0.0f, 80.0f, 80.0f});
    bg->setContentSize(size);
    bg->setPosition(winSize.width / 2, winSize.height / 2);
    m_mainLayer->addChild(bg);

    m_buttonMenu = CCMenu::create();
    m_mainLayer->addChild(m_buttonMenu);

    //create close window button
    auto CloseS = CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png");
    CloseS->setScale(1);

    auto CloseButton = CCMenuItemSpriteExtra::create(
        CloseS,
        nullptr,
        this,
        menu_selector(BackupsLayer::backButtonCallback)
    );
    CloseButton->setUserData(reinterpret_cast<void*>(this));

    m_buttonMenu->addChild(CloseButton);

    CloseButton->setPosition(-_w / 2.1f, _h / 2.1f);

    //create backups dir if doesnt exist
    std::string fileName = "\\Backups";
    std::filesystem::create_directory(Mod::get()->getSaveDir().string() + fileName);
    BackupsDir = Mod::get()->getSaveDir().string() + fileName;

    fileName += "\\Auto-Backups";
    std::filesystem::create_directory(Mod::get()->getSaveDir().string() + fileName);


    //create the backups list
    RefreshBackupsList();

    //external buttons

    //add backup button
    CCSprite* addBSprite = CCSprite::createWithSpriteFrameName("GJ_plusBtn_001.png");
    addButton = CCMenuItemSpriteExtra::create(
        addBSprite,
        nullptr,
        this,
        menu_selector(BackupsLayer::addButtonClicked)
    );
    addButton->setPosition({164, 54});
    m_buttonMenu->addChild(addButton);

    //add backup button
    auto deleteBSprite = CCSprite::createWithSpriteFrameName("GJ_deleteBtn_001.png");
    auto deleteButton = CCMenuItemSpriteExtra::create(
        deleteBSprite,
        nullptr,
        this,
        menu_selector(BackupsLayer::deleteButtonClicked)
    );
    deleteButton->setPosition({164, -54});
    m_buttonMenu->addChild(deleteButton);

    //create open folder button
    auto OFSprite = CCSprite::createWithSpriteFrameName("GJ_plainBtn_001.png");
    OFSprite->setScale(0.75f);
    auto OFSpriteFolder = CCSprite::createWithSpriteFrameName("folderIcon_001.png");
    OFSpriteFolder->setPosition(OFSprite->getContentSize() / 2);
    OFSpriteFolder->setScale(0.8f);
    OFSprite->addChild(OFSpriteFolder);

    auto OFButton = CCMenuItemSpriteExtra::create(
        OFSprite,
        nullptr,
        this,
        menu_selector(BackupsLayer::OpenFolder)
    );
    OFButton->setPosition({212, -105});
    m_buttonMenu->addChild(OFButton);

    //add settings button
    auto settingsBSprite = CCSprite::createWithSpriteFrameName("GJ_optionsBtn_001.png");
    settingsBSprite->setScale(0.7f);
    auto settingsButton = CCMenuItemSpriteExtra::create(
        settingsBSprite,
        nullptr,
        this,
        menu_selector(BackupsLayer::openSettings)
    );
    settingsButton->setPosition({209, 104});
    m_buttonMenu->addChild(settingsButton);

    //add mode button
    auto modeBSprite = CCSprite::createWithSpriteFrameName("GJ_plainBtn_001.png");
    normalsIcon = CCSprite::createWithSpriteFrameName("GJ_downloadsIcon_001.png");
    normalsIcon->setPosition(modeBSprite->getContentSize() / 2);
    normalsIcon->setScale(1.15f);
    modeBSprite->addChild(normalsIcon);
    autosIcon = CCSprite::createWithSpriteFrameName("diffIcon_auto_btn_001.png");
    autosIcon->setPosition(modeBSprite->getContentSize() / 2);
    autosIcon->setVisible(false);
    modeBSprite->addChild(autosIcon);
    auto modeButton = CCMenuItemSpriteExtra::create(
        modeBSprite,
        nullptr,
        this,
        menu_selector(BackupsLayer::changeViewMode)
    );
    modeButton->setPosition({164, 0});
    m_buttonMenu->addChild(modeButton);

    this->setKeypadEnabled(true);
    this->setTouchEnabled(true);

    return true;
}

void BackupsLayer::show(CCNode* _parent) {
    this->setZOrder(100);

    parent = _parent;

    m_mainLayer->runAction(CCEaseElasticOut::create(CCScaleTo::create(0.6f, 1.1f), 0.5f));
    this->runAction(CCFadeTo::create(0.14f, 100));
        
    _parent->addChild(this);  
}

void BackupsLayer::keyBackClicked() {
    this->setKeyboardEnabled(false);
    this->removeFromParentAndCleanup(true);
}

void BackupsLayer::backButtonCallback(CCObject* object) {
    keyBackClicked();
}

void BackupsLayer::addButtonClicked(CCObject* object){
    CreateBackupLayer::create(this)->show(this);
}

void BackupsLayer::deleteButtonClicked(CCObject* object){
    deleteWarning = FLAlertLayer::create(
        this,
        "Warning!",
        "Are you sure you want to delete\nall selected backups?",
        "No",
        "Yes"
    );
    deleteWarning->show();
}

void BackupsLayer::RefreshBackupsList(){
    if (list != nullptr){
        list->removeMeAndCleanup();
    }

    CCArray* BackupsList = CCArray::create();


    Result<std::vector<ghc::filesystem::path>> readBackups;
    std::string titleLol;
    std::string noBUMessage;

    if (displayingAutos){
        readBackups = file::readDirectory(BackupsDir + "\\Auto-Backups");
        titleLol = "Auto Backups";
        noBUMessage = "You don't have any\nAuto Backups";
    }
    else{
        readBackups = file::readDirectory(BackupsDir);
        titleLol = "Backups";
        noBUMessage = "You don't have any\nBackups";
    }

    for (int i = readBackups.value().size(); i--> 0;)
    {
        std::ifstream file;

        std::string dataPath = readBackups.value()[i].string();
        std::string OriginCellPath = dataPath;
        dataPath += "\\Backup.dat";
        file.open(dataPath);
        if (file){
            BackupCell* cell = BackupCell::create(OriginCellPath, this, displayingAutos);

            BackupsList->addObject(cell);
        }
    }
    
    ListView* ListV = ListView::create(BackupsList, 40, 308, 200);

    ListV->setScaleY(0.875f);
    ListV->setScaleX(0.89f);
    ListV->setPosition({-14, -9.5f});

    list = GJListLayer::create(ListV, titleLol.c_str(), {0, 0, 0, 0}, 280, 180, 2);

    m_mainLayer->addChild(list);
    handleTouchPriority(this);

    CCArray* listChilds = list->getChildren();

    static_cast<CCSprite*>(listChilds->objectAtIndex(0))->setScale(0.8f);
    CCSprite* topS = static_cast<CCSprite*>(listChilds->objectAtIndex(1));
    topS->setScale(0.8f);
    topS->setPosition({140, 191});

    CCLabelBMFont* label = static_cast<CCLabelBMFont*>(listChilds->objectAtIndex(4));
    label->setScale(0.7f);
    label->setPosition({140, 194});

    list->setPosition({101, 64});

    if (BackupsList->count() == 0){
        CCLabelBMFont* noBLavel = CCLabelBMFont::create(noBUMessage.c_str(), "bigFont.fnt");
        noBLavel->setAlignment(CCTextAlignment::kCCTextAlignmentCenter);
        noBLavel->setScale(0.725f);

        list->addChild(noBLavel);
        noBLavel->setPosition(list->getContentSize() / 2);
    }
}

void BackupsLayer::FLAlert_Clicked(FLAlertLayer* p0, bool p1){
    if (p0 == deleteWarning){
        if (p1){
            CCObject* obj;
            CCARRAY_FOREACH(list->m_listView->m_entries, obj){
                auto cell = static_cast<BackupCell*>(obj);
                if (cell){
                    if (cell->selected) std::filesystem::remove_all(cell->_folderPath);
                }
            }
            RefreshBackupsList();
        }
    }
    
}

void BackupsLayer::OpenFolder(CCObject* object){
    ShellExecuteA(NULL, "open", BackupsDir.c_str(), NULL, NULL, SW_SHOWDEFAULT);
}

void BackupsLayer::openSettings(CCObject* object){
    geode::openSettingsPopup(Mod::get());
}

void BackupsLayer::changeViewMode(CCObject* object){
    displayingAutos = !displayingAutos;
    if (displayingAutos){
        autosIcon->setVisible(true);
        normalsIcon->setVisible(false);
        addButton->setVisible(false);
    }
    else{
        autosIcon->setVisible(false);
        normalsIcon->setVisible(true);
        addButton->setVisible(true);
    }
    RefreshBackupsList();
}