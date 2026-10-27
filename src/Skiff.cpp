// Skiff: dual probability skipper for VCV Rack, 4 HP.
// Recreated from the published behaviour of the Ladik S-090 (see src/Skip.hpp).
#include <rack.hpp>
#include "Skip.hpp"
#include "Controls.hpp"
#include "Stencil.hpp"

using namespace rack;

Plugin* pluginInstance;

struct Skiff : Module {
	enum ParamId { ENUMS(PROB_PARAM, 2), PARAMS_LEN };
	enum InputId { ENUMS(CV_INPUT, 2), ENUMS(GATE_INPUT, 2), INPUTS_LEN };
	enum OutputId { ENUMS(GATE_OUTPUT, 2), OUTPUTS_LEN };
	enum LightId { ENUMS(IN_LIGHT, 2), ENUMS(OUT_LIGHT, 2), LIGHTS_LEN };

	dsp::SchmittTrigger gates[2];
	bool passing[2] = {};
	skiff::Decider deciders[2];
	skiff::Random rng;
	// the two jumpers on the back of the original
	bool exclusive = false;
	bool smooth = false;
	int theme = 0;

	Skiff() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		for (int i = 0; i < 2; i++) {
			configParam(PROB_PARAM + i, 0.f, 1.f, 0.5f, string::f("Probability %d", i + 1), "%", 0.f, 100.f);
			configInput(CV_INPUT + i, string::f("Probability %d CV (0..5 V, attenuated by the knob)", i + 1));
			configInput(GATE_INPUT + i, i == 0 ? "Gate 1" : "Gate 2 (normalled to gate 1)");
			configOutput(GATE_OUTPUT + i, string::f("Gate %d", i + 1));
		}
		rng.state = random::u32() | 1u;
	}

	float prob(int i) {
		return skiff::probability(params[PROB_PARAM + i].getValue(), inputs[CV_INPUT + i].isConnected(), inputs[CV_INPUT + i].getVoltage());
	}

	void process(const ProcessArgs& args) override {
		bool shared = !inputs[GATE_INPUT + 1].isConnected();
		float v[2] = {inputs[GATE_INPUT].getVoltage(), shared ? inputs[GATE_INPUT].getVoltage() : inputs[GATE_INPUT + 1].getVoltage()};
		bool edge[2], high[2];
		for (int i = 0; i < 2; i++) {
			edge[i] = gates[i].process(v[i], 0.5f, 1.5f);
			high[i] = gates[i].isHigh();
		}

		if (exclusive && shared) {
			// one input pulse goes to one output at most; channel 1 decides first
			if (edge[0]) {
				passing[0] = deciders[0].decide(prob(0), smooth, rng);
				passing[1] = !passing[0] && deciders[1].decide(prob(1), smooth, rng);
			}
		}
		else {
			for (int i = 0; i < 2; i++) {
				if (!edge[i])
					continue;
				bool blocked = exclusive && passing[1 - i] && high[1 - i];  // the other output is busy
				passing[i] = !blocked && deciders[i].decide(prob(i), smooth, rng);
			}
		}

		for (int i = 0; i < 2; i++) {
			if (!high[i])
				passing[i] = false;
			bool out = passing[i] && high[i];
			outputs[GATE_OUTPUT + i].setVoltage(out ? 10.f : 0.f);
			lights[IN_LIGHT + i].setBrightnessSmooth(high[i] ? 1.f : 0.f, args.sampleTime);
			lights[OUT_LIGHT + i].setBrightnessSmooth(out ? 1.f : 0.f, args.sampleTime);
		}
	}

	json_t* dataToJson() override {
		json_t* root = json_object();
		json_object_set_new(root, "exclusive", json_boolean(exclusive));
		json_object_set_new(root, "smooth", json_boolean(smooth));
		json_object_set_new(root, "theme", json_integer(theme));
		return root;
	}

	void dataFromJson(json_t* root) override {
		if (json_t* j = json_object_get(root, "exclusive"))
			exclusive = json_boolean_value(j);
		if (json_t* j = json_object_get(root, "smooth"))
			smooth = json_boolean_value(j);
		if (json_t* j = json_object_get(root, "theme"))
			theme = clamp((int) json_integer_value(j), 0, 1);
	}
};

// ---------------------------------------------------------------- themes
// betobullet stencil (docs/art/STYLE.md): theme 0 = black panel, 1 = white panel
using bb::THEME_NAMES;

namespace fms {
const Colors& colors(const engine::Module* m) {
	static const Colors black = bb::controlColors(0), white = bb::controlColors(1);
	int t = m ? static_cast<const Skiff*>(m)->theme : 0;
	return t == 1 ? white : black;
}
}  // namespace fms

static std::shared_ptr<window::Font> pluginFont(const char* file) {
	std::shared_ptr<window::Font> f = APP->window->loadFont(asset::plugin(pluginInstance, std::string("res/fonts/") + file));
	if (!f)
		f = APP->window->loadFont(asset::system("res/fonts/DejaVuSans.ttf"));
	return f;
}

// ---------------------------------------------------------------- layout (mm), 4 HP
static const float W = 20.32f;
static const float CX = W / 2.f;
// after concept light v1: one outlined group per channel, the output on a black plate inside it
static const float CH_TOP[2] = {14.0f, 67.2f};  // top of each channel group
static const float GROUP_H = 51.6f;
static const float KNOB_DY = 8.0f, CV_DY = 21.0f, IN_DY = 30.8f, OUT_DY = 43.4f;
static const float PLATE_DY = 38.4f, PLATE_H = 11.4f;
static const float JACK_X = 9.4f, LED_X = 16.0f;  // jacks a little left, LEDs beside them
static const float KNOB_R = 5.0f;

/** Small round LED in the accent colour. */
struct Led : TransparentWidget {
	Skiff* module = nullptr;
	int lightId = 0;

	float level() {
		return module ? module->lights[lightId].getBrightness() : 0.f;
	}

	void draw(const DrawArgs& args) override {
		const fms::Colors& c = fms::colors(module);
		float r = box.size.x / 2;
		fms::circle(args.vg, r, r, r * 0.9f, fms::mix(c.jackDark, c.btnOn, level()), c.outline, 0.8f);
	}

	void drawLayer(const DrawArgs& args, int layer) override {
		if (layer != 1 || level() < 0.02f)
			return;
		const fms::Colors& c = fms::colors(module);
		NVGcontext* vg = args.vg;
		float r = box.size.x / 2, b = level();
		fms::circle(vg, r, r, r * 0.8f, fms::mix(c.jackDark, c.btnOn, b), c.btnOn, 0.f);
		nvgBeginPath(vg);
		nvgCircle(vg, r, r, r * 2.6f);
		nvgFillPaint(vg, nvgRadialGradient(vg, r, r, r * 0.6f, r * 2.6f, nvgRGBAf(c.btnOn.r, c.btnOn.g, c.btnOn.b, 0.35f * b),
		                                   nvgRGBAf(c.btnOn.r, c.btnOn.g, c.btnOn.b, 0.f)));
		nvgFill(vg);
	}
};

struct PanelArt : Widget {
	Skiff* module = nullptr;

	void draw(const DrawArgs& args) override {
		bb::Ink k = bb::ink(module ? module->theme : 0);
		NVGcontext* vg = args.vg;
		nvgBeginPath(vg);
		nvgRect(vg, 0, 0, box.size.x, box.size.y);
		nvgFillColor(vg, k.bg);
		nvgFill(vg);

		bb::rule(vg, 2.4f, W - 2.4f, CH_TOP[1] - 1.0f, k);
		for (int i = 0; i < 2; i++)
			bb::outRing(vg, Vec(JACK_X, CH_TOP[i] + OUT_DY), k);

		std::shared_ptr<window::Font> stencil = pluginFont("BlackOpsOne-Regular.ttf");
		std::shared_ptr<window::Font> bold = pluginFont("BebasNeue-Regular.ttf");
		if (!stencil || !bold)
			return;
		bb::nameplate(vg, CX, 7.0f, "SKIFF", stencil->handle, k, 3.6f, bb::MARK_PULSES, 15.0f, 3.6f);
		nvgFontFaceId(vg, bold->handle);
		nvgFontSize(vg, mm2px(4.8f));
		nvgTextLetterSpacing(vg, mm2px(0.08f));
		for (int i = 0; i < 2; i++) {
			float top = CH_TOP[i];
			bb::label(vg, Vec(CX, top + KNOB_DY + KNOB_R + 2.6f), string::f("PROB %d", i + 1).c_str(), k.fg);
			nvgFontSize(vg, mm2px(3.6f));
			bb::label(vg, Vec(JACK_X, top + CV_DY + 5.2f), "CV", k.fg);
			bb::label(vg, Vec(JACK_X, top + IN_DY + 5.2f), i == 0 ? "IN" : "IN 2", k.fg);
			bb::label(vg, Vec(JACK_X, top + OUT_DY + 6.6f), "OUT", k.red);
			nvgFontSize(vg, mm2px(4.8f));
		}
		nvgTextLetterSpacing(vg, 0.f);
	}
};

struct SkiffWidget : ModuleWidget {
	SkiffWidget(Skiff* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Skiff.svg")));

		PanelArt* art = new PanelArt;
		art->module = module;
		art->box.size = box.size;
		addChild(art);

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		for (int i = 0; i < 2; i++) {
			float top = CH_TOP[i];
			addParam(fms::knob(mm2px(Vec(CX, top + KNOB_DY)), module, Skiff::PROB_PARAM + i, KNOB_R));
			addInput(createInputCentered<fms::Jack>(mm2px(Vec(JACK_X, top + CV_DY)), module, Skiff::CV_INPUT + i));
			addInput(createInputCentered<fms::Jack>(mm2px(Vec(JACK_X, top + IN_DY)), module, Skiff::GATE_INPUT + i));
			addOutput(createOutputCentered<fms::Jack>(mm2px(Vec(JACK_X, top + OUT_DY)), module, Skiff::GATE_OUTPUT + i));
			// LEDs beside the gate jacks, like the original
			for (int j = 0; j < 2; j++) {
				Led* led = new Led;
				led->module = module;
				led->lightId = (j == 0 ? Skiff::IN_LIGHT : Skiff::OUT_LIGHT) + i;
				led->box.size = mm2px(Vec(2.2f, 2.2f));
				Vec c = mm2px(Vec(LED_X, top + (j == 0 ? IN_DY : OUT_DY)));
				led->box.pos = c.minus(led->box.size.div(2));
				addChild(led);
			}
		}
	}

	void appendContextMenu(Menu* menu) override {
		Skiff* m = getModule<Skiff>();
		if (!m)
			return;
		menu->addChild(new MenuSeparator);
		menu->addChild(createMenuLabel("Jumpers"));
		menu->addChild(createBoolPtrMenuItem("Exclusive (one output per pulse)", "", &m->exclusive));
		menu->addChild(createBoolPtrMenuItem("Regular randomness (0 % / 100 % not exact)", "", &m->smooth));
		menu->addChild(createIndexPtrSubmenuItem("Theme", std::vector<std::string>(THEME_NAMES, THEME_NAMES + 2), &m->theme));
	}
};

Model* modelSkiff = createModel<Skiff, SkiffWidget>("Skiff");

void init(Plugin* p) {
	pluginInstance = p;
	p->addModel(modelSkiff);
}
