#include "./includes.h"
#include "./FastPatch.h"

struct MainLevelData {
	std::string name;
	int difficulty;
	int stars;
	std::string song;
};



class MainLevelsCustomizerManager {

	enum DataLoadingResult {
		OK,
		FileNotFound,
		ParsingError,
		LevelsCountError,
		LevelNameLengthError,
		LevelStarsCountError,
		LevelDifficultyError,
	};

	std::vector<MainLevelData*> levelsData;
	bool unlockDemons = false;
	DataLoadingResult loadingStatus;
	static MainLevelsCustomizerManager* instance;

	bool isInMainLevels = false;

	void init() {
		loadingStatus = loadData();
		if (loadingStatus == OK) {
			FastPatch::make("0x1FC352", "E9 AA 00 00 00");//load unfailed
			auto byteVector = std::vector<uint8_t>();
			byteVector.push_back(levelsData.size() + 1);
			FastPatch::patch(0x1859AE, byteVector);//levels count patch
			if (unlockDemons) {
				FastPatch::make("0x187A1D", "E9 E7 01 00 00");
				FastPatch::make("0x188CE1", "E9 8A 00 00 00 90");
			}
		}
	}

	DataLoadingResult loadData() {
		std::ifstream file("Resources/levelCustomizer.json");
		if (!file) return FileNotFound;
		std::ostringstream buffer;
		buffer << file.rdbuf();
		std::string fileContent = buffer.str();

		file.close();
		try {
			auto root = nlohmann::json::parse(fileContent);

			if (!root.contains("settings") || !root["settings"].is_object() || !root.contains("levels") || !root["levels"].is_array()) return ParsingError;

			if (!root["settings"].contains("unlockDemons") || !root["settings"]["unlockDemons"].is_boolean()) return ParsingError;
			unlockDemons = root["settings"]["unlockDemons"].get<bool>();

			if (root["levels"].size() < 1 || root["levels"].size() > 126) return LevelsCountError;

			for (auto level : root["levels"]) {
				if (!level.contains("name") || !level["name"].is_string()
					|| !level.contains("difficulty") || !level["difficulty"].is_number_integer()
					|| !level.contains("stars") || !level["stars"].is_number_integer()
					|| !level.contains("song") || !level["song"].is_string()) return ParsingError;

				auto newLevel = new MainLevelData();
				newLevel->name = level["name"];
				newLevel->difficulty = level["difficulty"].get<int>();
				newLevel->stars = level["stars"].get<int>();
				newLevel->song = level["song"];

				if (newLevel->name.size() > 100 || newLevel->name.size() == 0) return LevelNameLengthError;
				if (newLevel->difficulty < 1 || newLevel->difficulty > 6) return LevelDifficultyError;
				if (newLevel->stars <= 0 || newLevel->stars > 9999) return LevelStarsCountError;

				levelsData.push_back(newLevel);
			}
		}
		catch (...) {
			
			return ParsingError;
		}
		return OK;
	}

	MainLevelsCustomizerManager() {};
public:

	void onMenuLayer(MenuLayer* layer) {

		if (loadingStatus != OK) {

			std::string errorText;
			switch (loadingStatus) {
			case FileNotFound:
				errorText = "Can't find 'levelCustomizer.json' in ./Resources";
				break;
			case ParsingError:
				errorText = "Can't parse 'levelCustomizer.json'";
				break;
			case LevelsCountError:
				errorText = "Too many or too few levels in 'levelCustomizer.json'";
				break;
			case LevelNameLengthError:
				errorText = "Levelname is too long or empty 'levelCustomizer.json'";
				break;
			case LevelDifficultyError:
				errorText = "Difficulty should be between 1 and 6 in 'levelCustomizer.json'";
				break;
			case LevelStarsCountError:
				errorText = "Too many stars for level in 'levelCustomizer.json'";
				break;
			}

			auto size = CCDirector::sharedDirector()->getWinSize();

			auto errorLabel = CCLabelBMFont::create(errorText.c_str(), "bigFont.fnt");
			errorLabel->setColor({ 255, 0, 0 });
			errorLabel->setScale(0.4);
			errorLabel->setPosition({ size.width / 2, size.height - 10 });
			layer->addChild(errorLabel);
		}
	}

	void onLevelPage_customSetup(void* self, GJGameLevel* lvl) {
		if (loadingStatus != OK || !isInMainLevels) return;

		if (lvl->levelID_rand - lvl->levelID_seed != -1) {
			auto id = lvl->levelID_rand - lvl->levelID_seed;

			bool brokenValue = id < 1 || id > levelsData.size();

			lvl->levelName = brokenValue ? "Error :/" : levelsData[id - 1]->name;

			lvl->stars_rand = 1000;
			lvl->stars_seed = brokenValue ? 999 : (1000 - levelsData[id - 1]->stars);

			lvl->difficulty = brokenValue ? 1 : levelsData[id - 1]->difficulty;
		}
	}

	std::string& onGetAudioFileName(std::string& self, int id) {
		if (loadingStatus != OK || !isInMainLevels) return self;

		if (id < 0 || id > levelsData.size() - 1) return self;
		self = levelsData[id]->song;
		
		return self;
	}

	void onLevelSelectLayer_init() {
		isInMainLevels = true;
	}

	void onLevelSelectLayer_onBack() {
		isInMainLevels = false;
	}


	static MainLevelsCustomizerManager* getInstance() {
		if (!instance) {
			instance = new MainLevelsCustomizerManager();
			instance->init();
		}
		return instance;
	}

};
MainLevelsCustomizerManager* MainLevelsCustomizerManager::instance = nullptr;



bool(__thiscall* MenuLayer_init)(MenuLayer* self);
bool __fastcall MenuLayer_init_H(MenuLayer* self, void*) {
	if (!MenuLayer_init(self)) return false;
	MainLevelsCustomizerManager::getInstance()->onMenuLayer(self);
	return true;
}


bool(__thiscall* LevelSelectLayer_init)(CCLayer* self, int idk);
bool __fastcall LevelSelectLayer_init_H(CCLayer* self, void*, int idk) {
	MainLevelsCustomizerManager::getInstance()->onLevelSelectLayer_init();
	if (!LevelSelectLayer_init(self, idk)) return false;

	return true;
}

void(__thiscall* LevelSelectLayer_onBack)(CCLayer* self, CCObject* obj);
void __fastcall LevelSelectLayer_onBack_H(CCLayer* self, void*, CCObject* obj) {
	MainLevelsCustomizerManager::getInstance()->onLevelSelectLayer_onBack();
	LevelSelectLayer_onBack(self, obj);
}

void(__thiscall * LevelPage_customSetup)(void * self, GJGameLevel * lvl);
void __fastcall LevelPage_customSetup_H(void* self, void*, GJGameLevel* lvl) {
	MainLevelsCustomizerManager::getInstance()->onLevelPage_customSetup(self, lvl);
	LevelPage_customSetup(self, lvl);
}

std::string& (__fastcall * LevelTools_getAudioFileName)(std::string & self, int id);
std::string& __fastcall LevelTools_getAudioFileName_H(std::string& self, int id) {
	auto ret = LevelTools_getAudioFileName(self, id);
	MainLevelsCustomizerManager::getInstance()->onGetAudioFileName(self, id);
	return ret;
}

void inject() {
#if _WIN32
	auto base = reinterpret_cast<uintptr_t>(GetModuleHandle(0));

	MH_CreateHook(
		reinterpret_cast<void*>(base + 0x1907b0),
		reinterpret_cast<void*>(&MenuLayer_init_H),
		reinterpret_cast<void**>(&MenuLayer_init)
	);

	MH_CreateHook(
		reinterpret_cast<void*>(base + 0x1855A0),
		reinterpret_cast<void*>(&LevelSelectLayer_init_H),
		reinterpret_cast<void**>(&LevelSelectLayer_init)
	);

	MH_CreateHook(
		reinterpret_cast<void*>(base + 0x1864B0),
		reinterpret_cast<void*>(&LevelSelectLayer_onBack_H),
		reinterpret_cast<void**>(&LevelSelectLayer_onBack)
	);

	MH_CreateHook(
		reinterpret_cast<void*>(base + 0x187220),
		reinterpret_cast<void*>(&LevelPage_customSetup_H),
		reinterpret_cast<void**>(&LevelPage_customSetup)
	);

	MH_CreateHook(
		reinterpret_cast<void*>(base + 0x189FA0),
		reinterpret_cast<void*>(&LevelTools_getAudioFileName_H),
		reinterpret_cast<void**>(&LevelTools_getAudioFileName)
	);

	MH_EnableHook(MH_ALL_HOOKS);
#endif
}

#if _WIN32
WIN32CAC_ENTRY(inject)
#endif