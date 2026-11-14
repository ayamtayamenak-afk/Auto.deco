#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/ScrollLayer.hpp>
#include <random>
#include <vector>

using namespace geode::prelude;

// ---------------------------------------------------------------------------
// 23 style deco. Ubah / tambah sesuka hati: warna, ID objek, kepadatan, dll.
// ---------------------------------------------------------------------------
struct DecoStyle {
	const char* name;
	ccColor3B c1;            // warna utama (channel 901)
	ccColor3B c2;            // warna aksen (channel 902)
	std::vector<int> ids;    // ID objek yang dipakai sebagai deco
	int density;             // jumlah deco per objek
	float spread;            // jarak sebar (unit editor, 30 = 1 blok)
	float minScale, maxScale;
	bool rotate;
};

static const std::vector<DecoStyle>& getStyles() {
	static const std::vector<DecoStyle> styles = {
		{"Nightmare",  {40,0,60},     {200,0,120},   {1,2,3,40},      4, 70.f, .3f, 1.1f, true },
		{"Hell",       {120,0,0},     {255,90,0},    {1,3,5,40},      5, 80.f, .3f, 1.3f, true },
		{"Heaven",     {255,250,220}, {120,200,255}, {1,2,40},        3, 90.f, .4f, 1.2f, false},
		{"Ice",        {120,220,255}, {230,250,255}, {1,4,6},         4, 60.f, .3f, 1.0f, true },
		{"Forest",     {20,100,40},   {120,200,60},  {1,2,5,7},       5, 75.f, .4f, 1.2f, true },
		{"Inferno",    {255,60,0},    {255,200,0},   {1,3,40},        6, 85.f, .3f, 1.4f, true },
		{"Space",      {10,10,50},    {120,100,255}, {1,2,4},         3, 120.f,.2f, 0.9f, false},
		{"Neon",       {0,255,200},   {255,0,200},   {1,40},          4, 60.f, .3f, 1.0f, false},
		{"Cyber",      {0,150,255},   {255,230,0},   {1,2,3,4},       4, 65.f, .3f, 1.1f, false},
		{"Ocean",      {0,70,160},    {0,220,220},   {1,4,6,7},       4, 90.f, .4f, 1.2f, true },
		{"Desert",     {210,160,80},  {255,220,140}, {1,2,5},         3, 80.f, .4f, 1.3f, false},
		{"Candy",      {255,120,200}, {120,230,255}, {1,3,6,40},      4, 70.f, .4f, 1.1f, true },
		{"Toxic",      {60,255,0},    {120,0,160},   {1,4,5},         5, 70.f, .3f, 1.2f, true },
		{"Lava",       {160,20,0},    {255,140,0},   {1,3,5,40},      5, 80.f, .3f, 1.3f, true },
		{"Crystal",    {160,100,255}, {200,240,255}, {1,6,7},         4, 65.f, .3f, 1.0f, true },
		{"Storm",      {50,60,90},    {200,210,255}, {1,2,4,40},      4, 95.f, .3f, 1.2f, true },
		{"Sunset",     {255,90,60},   {255,190,90},  {1,2,3},         3, 100.f,.4f, 1.3f, false},
		{"Galaxy",     {60,0,120},    {255,120,220}, {1,2,4,6},       4, 110.f,.2f, 1.0f, true },
		{"Void",       {0,0,0},       {110,0,200},   {1,3,40},        3, 90.f, .3f, 1.4f, true },
		{"Steampunk",  {140,90,40},   {210,170,90},  {1,2,5,7},       4, 60.f, .4f, 1.1f, false},
		{"Retro",      {255,0,120},   {0,220,255},   {1,2,3,4},       3, 60.f, .4f, 1.0f, false},
		{"Glitch",     {0,255,100},   {255,0,60},    {1,3,40},        6, 50.f, .2f, 1.5f, true },
		{"Mechanical", {90,100,110},  {255,160,0},   {1,2,4,5,7},     4, 55.f, .4f, 1.1f, false},
	};
	return styles;
}

// ---------------------------------------------------------------------------
// Logika auto deco
// ---------------------------------------------------------------------------
static void makeColorTrigger(LevelEditorLayer* lel, int channel, ccColor3B color, float y) {
	auto obj = lel->createObject(899, ccp(0.f, y), false); // 899 = Color Trigger
	if (auto trig = typeinfo_cast<EffectGameObject*>(obj)) {
		trig->m_targetGroupID = channel;
		trig->m_triggerTargetColor = color;
		trig->m_duration = 0.f;
	}
}

static void applyStyle(EditorUI* ui, DecoStyle const& st) {
	auto lel = ui->m_editorLayer;

	CCArray* targets = ui->getSelectedObjects();
	if (!targets || targets->count() == 0) targets = lel->m_objects;
	if (!targets || targets->count() == 0) {
		FLAlertLayer::create("Auto Deco", "Level masih kosong. Taruh beberapa objek dulu.", "OK")->show();
		return;
	}

	// Simpan posisi dulu, karena membuat objek mengubah daftar objek level.
	std::vector<CCPoint> origins;
	for (auto obj : CCArrayExt<GameObject*>(targets)) {
		origins.push_back(obj->getPosition());
	}
	// Batasi supaya editor tidak lag (maks ~400 titik sumber).
	size_t step = std::max<size_t>(1, origins.size() / 400);

	makeColorTrigger(lel, 901, st.c1, 345.f);
	makeColorTrigger(lel, 902, st.c2, 375.f);

	std::random_device rd;
	std::mt19937 rng(rd());
	std::uniform_real_distribution<float> offs(-1.f, 1.f);
	std::uniform_real_distribution<float> rot(0.f, 360.f);

	int created = 0;
	for (size_t i = 0; i < origins.size(); i += step) {
		for (int k = 0; k < st.density; ++k) {
			int id = st.ids[rng() % st.ids.size()];
			CCPoint pos = {
				origins[i].x + offs(rng) * st.spread,
				origins[i].y + offs(rng) * st.spread
			};
			auto deco = lel->createObject(id, pos, false);
			if (!deco) continue;

			std::uniform_real_distribution<float> sc(st.minScale, st.maxScale);
			float s = sc(rng);
			deco->updateCustomScaleX(s);
			deco->updateCustomScaleY(s);
			if (st.rotate) deco->setRotation(rot(rng));
			deco->m_isNoTouch = true; // deco tidak boleh membunuh pemain
			if (deco->m_baseColor) {
				deco->m_baseColor->m_colorID = (k % 2 == 0) ? 901 : 902;
			}
			++created;
		}
	}

	Notification::create(
		fmt::format("Auto Deco {}: {} objek dibuat", st.name, created),
		NotificationIcon::Success
	)->show();
}

// ---------------------------------------------------------------------------
// Popup menu Auto Deco
// ---------------------------------------------------------------------------
class AutoDecoPopup : public Popup<EditorUI*> {
protected:
	EditorUI* m_editorUI = nullptr;

	bool setup(EditorUI* ui) override {
		m_editorUI = ui;
		this->setTitle("Auto Deco - AJ");

		auto const& styles = getStyles();
		auto size = m_mainLayer->getContentSize();

		float viewW = size.width - 30.f;
		float viewH = size.height - 60.f;
		auto scroll = ScrollLayer::create({viewW, viewH});
		scroll->setPosition({15.f, 15.f});
		m_mainLayer->addChild(scroll);

		const int cols = 2;
		const float rowH = 38.f;
		int rows = (static_cast<int>(styles.size()) + cols - 1) / cols;
		float contentH = std::max(viewH, rows * rowH + 10.f);
		scroll->m_contentLayer->setContentSize({viewW, contentH});

		auto menu = CCMenu::create();
		menu->setContentSize({viewW, contentH});
		menu->setPosition({0.f, 0.f});
		scroll->m_contentLayer->addChild(menu);

		for (int i = 0; i < static_cast<int>(styles.size()); ++i) {
			int col = i % cols, row = i / cols;
			auto label = fmt::format("{}. {}", i + 1, styles[i].name);
			auto spr = ButtonSprite::create(label.c_str(), 130, true, "bigFont.fnt", "GJ_button_01.png", 28.f, .6f);
			auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(AutoDecoPopup::onStyle));
			btn->setTag(i);
			btn->setPosition({viewW * (col + 0.5f) / cols, contentH - 24.f - row * rowH});
			menu->addChild(btn);
		}
		scroll->moveToTop();
		return true;
	}

	void onStyle(CCObject* sender) {
		int idx = static_cast<CCNode*>(sender)->getTag();
		auto const& styles = getStyles();
		if (idx < 0 || idx >= static_cast<int>(styles.size())) return;
		applyStyle(m_editorUI, styles[idx]);
		this->onClose(nullptr);
	}

public:
	static AutoDecoPopup* create(EditorUI* ui) {
		auto ret = new AutoDecoPopup();
		if (ret->initAnchored(320.f, 260.f, ui)) {
			ret->autorelease();
			return ret;
		}
		delete ret;
		return nullptr;
	}
};

// ---------------------------------------------------------------------------
// Tombol "AJ" di bagian atas editor
// ---------------------------------------------------------------------------
class $modify(AJEditorUI, EditorUI) {
	bool init(LevelEditorLayer* lel) {
		if (!EditorUI::init(lel)) return false;

		auto win = CCDirector::get()->getWinSize();
		auto spr = ButtonSprite::create("AJ");
		spr->setScale(.7f);
		auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(AJEditorUI::onAutoDeco));

		auto menu = CCMenu::create();
		menu->setPosition({50.f, win.height - 25.f}); // geser kalau bentrok dengan tombol lain
		menu->addChild(btn);
		this->addChild(menu, 100);
		return true;
	}

	void onAutoDeco(CCObject*) {
		AutoDecoPopup::create(this)->show();
	}
};
