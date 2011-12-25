// 3D World - OpenAL Interface Code Header
// by Frank Gennari
// 9/1/11
#include "3DWorld.h"


// static sounds
enum {SOUND_BURNING=0, SOUND_RAIN1, SOUND_WIND1, SOUND_EXPLODE, SOUND_GUNSHOT, SOUND_SHOTGUN, SOUND_FIREBALL, SOUND_DROWN, SOUND_SCREAM1, SOUND_SCREAM2,
	SOUND_GLASS, SOUND_DRILL, SOUND_ROCKET, SOUND_ITEM, SOUND_POWERUP, SOUND_ALERT, SOUND_SQUISH, SOUND_SQUISH2, SOUND_SPLAT1, SOUND_SPLASH1,
	SOUND_SPLASH2, SOUND_WATER, SOUND_THUNDER, SOUND_BOING, SOUND_SWING, SOUND_HISS, SOUND_DOH, SOUND_HURT, SOUND_DEATH, SOUND_AGONY,
	SOUND_SCARED, SOUND_GASP, SOUND_SCREAM3, SOUND_SQUEAL, SOUND_RICOCHET, NUM_SOUNDS};

// looping sounds
enum {SOUND_LOOP_FIRE, SOUND_LOOP_RAIN, SOUND_LOOP_WIND, NUM_LOOP_SOUNDS};


struct openal_orient {

	vector3d at, up;
	openal_orient() {}
	openal_orient(vector3d const &at_, vector3d const &up_) : at(at_), up(up_) {}
};


struct sound_params_t {

	point pos;
	float gain, pitch;
	bool rel_to_listener;

	sound_params_t(point const &P=all_zeros, float g=1.0, float p=1.0, bool r=0)
		: pos(P), gain(g), pitch(p), rel_to_listener(r) {}
	float get_loudness() const {return gain/max(SMALL_NUMBER, distance_to_camera(pos));}
};


struct delayed_sound_t : public sound_params_t {

	int id, time;

	delayed_sound_t() : time(0) {}
	delayed_sound_t(sound_params_t const &p, int i, int t) : sound_params_t(p), id(i), time(t) {}
};


class openal_buffer {

	unsigned buffer;
	float time;

public:
	openal_buffer(unsigned buffer_=0) : buffer(buffer_), time(0.0) {}
	//~openal_buffer() {free();}
	bool load_check();
	bool load_from_file(std::string const &fn);
	bool load_from_file_std_path(std::string const &fn);
	bool load_from_memory(void const *data, size_t length);
	bool load_from_waveform(int waveshape, float frequency, float phase, float duration);
	bool is_valid() const {return (buffer > 0);}
	unsigned get_buffer_ix() const {return buffer;}
	void alloc();
	void free();
};


class buffer_manager_t {

	vector<openal_buffer> buffers;

public:
	openal_buffer &get_buffer(unsigned id) {assert(id < buffers.size()); return buffers[id];}

	unsigned add_buffer(bool alloc) {
		unsigned const ix(buffers.size());
		buffers.push_back(openal_buffer());
		if (alloc) buffers.back().alloc();
		return ix;
	}
	unsigned add_file_buffer(std::string const &fn);

	void clear() {
		for (unsigned i = 0; i < buffers.size(); ++i) buffers[i].free();
		buffers.clear();
	}
};


class openal_source {

	unsigned source;
	sound_params_t params;

public:
	openal_source(unsigned source_=0) : source(source_) {}
	//~openal_source() {free();}
	bool is_valid  () const {return (source > 0);}
	bool is_playing() const;
	bool is_active () const;
	float get_loudness() const {return (is_active() ? params.get_loudness() : 0.0);}
	
	void alloc();
	void free();
	void setup(openal_buffer const &buffer, point const &pos, float gain=1.0, float pitch=1.0,
		bool looping=0, bool rel_to_listener=0, vector3d const &vel=zero_vector);
	void set_buffer(openal_buffer const &buffer) {set_buffer_ix(buffer.get_buffer_ix());}
	void set_buffer_ix(unsigned buffer_ix);
	void blocking_play() const;
	void play_if_not_playing() const;
	void play()   const;
	void stop()   const;
	void pause()  const;
	void rewind() const;
};


class source_manager_t {

	vector<openal_source> sources;
	unsigned next_source;

public:
	std::set<unsigned> used_this_frame;

	source_manager_t() : next_source(0) {}
	void create_channels(unsigned num_channels);
	unsigned new_source();
	openal_source &get_least_loud_source();
	openal_source &get_oldest_source();
	openal_source &get_inactive_source();
	openal_source &get_source(unsigned id) {assert(id < sources.size()); return sources[id];}
	void clear();
	bool is_playing   (unsigned id) const {assert(id < sources.size()); return sources[id].is_playing();}
	void play_source  (unsigned id) const {assert(id < sources.size()); sources[id].play  ();}
	void stop_source  (unsigned id) const {assert(id < sources.size()); sources[id].stop  ();}
	void pause_source (unsigned id) const {assert(id < sources.size()); sources[id].pause ();}
	void rewind_source(unsigned id) const {assert(id < sources.size()); sources[id].rewind();}
};


void set_sound_loop_state(unsigned id, bool play);
void setup_openal_listener(point const &pos, vector3d const &vel, openal_orient const &orient);
void set_openal_listener_as_player();
void gen_sound(unsigned id, point const &pos, float gain=1.0, float pitch=1.0, bool rel_to_listener=0, vector3d const &vel=zero_vector);
void gen_delayed_sound(float delay, unsigned id, point const &pos, float gain=1.0, float pitch=1.0, bool rel_to_listener=0); // no vel
void proc_delayed_sounds();
void init_openal(int &argc, char** argv);


