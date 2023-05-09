/**
  ******************************************************************************
  * @author		Anton Houzich
  * @version	V2.0.0
  * @date		9-May-2023
  * @mail		houzich_anton@mail.ru
  * discussion  https://t.me/BRUTE_FORCE_CRYPTO_WALLET
  ******************************************************************************
  */
#include "main.h"
#include <sstream>
#include <iomanip>
#include <iostream>
#include <omp.h>
#include <set>
#include <random>
#include <fstream>
#include <filesystem>

#include "../BruteForceMnemonic/stdafx.h"
#include "tools.h"
#include "utils.h"
#include "base58.h"
#include "segwit_addr.h"




namespace tools {


	void generateRandomWordsIndex(uint16_t* buff, size_t len) {
		std::random_device rd;
		std::uniform_int_distribution<uint16_t> distr;
		std::mt19937 eng(rd());

		for (int i = 0; i < len; i++)
		{
			buff[i] = distr(eng) % 1626;
		}
	}

	int pushToMemory(uint8_t* addr_buff, std::vector<std::string>& lines, int max_len) {
		int err = 0;
		for (int x = 0; x < lines.size(); x++) {
			const std::string line = lines[x];
			err = hexStringToBytes(line, &addr_buff[max_len * x], max_len);
			if (err != 0) {
				std::cerr << "\n!!!ERROR HASH160 TO BYTES: " << line << std::endl;
				return err;
			}
		}
		return err;
	}

	int readAllTables(tableStruct* tables, std::string path, std::string prefix, size_t *num_addresses_in_tables)
	{
		int ret = 0;
		std::string num_tables;
		size_t all_lines = 0;
#pragma omp parallel for 
		for (int x = 0; x < 256; x++) {

			std::string table_name = byteToHexString(x);

			std::string file_path = path + "\\" + prefix + table_name + ".csv";

			std::ifstream inFile(file_path);
			int64_t cnt_lines = std::count(std::istreambuf_iterator<char>(inFile), std::istreambuf_iterator<char>(), '\n');
			inFile.close();
			if (cnt_lines != 0) {
				tables[x].table = (uint32_t*)malloc(cnt_lines * 20);
				if (tables[x].table == NULL) {
					printf("Error: malloc failed to allocate buffers.Size %llu. From file %s\n", (unsigned long long int)(cnt_lines * 20), file_path.c_str());
					inFile.close();
					ret = -1;
					break;
				}
				tables[x].size = (uint32_t)_msize((void*)tables[x].table);
				memset((uint8_t*)tables[x].table, 0, cnt_lines * 20);
				inFile.open(file_path, std::ifstream::in);
				if (inFile.is_open())
				{
					std::vector<std::string> lines;
					std::string line;
					while (getline(inFile, line)) {
						lines.push_back(line);
					}

					ret = pushToMemory((uint8_t*)tables[x].table, lines, 20);
					if (ret != 0) {
						std::cerr << "\n!!!ERROR push_to_memory, file: " << file_path << std::endl;
						ret = -1;
						inFile.close();
						break;
					}

					if (cnt_lines != lines.size()) {
						std::cout << "cnt_lines != lines.size(): cnt_lines = " << cnt_lines << " lines.size() = " << lines.size() << std::endl;
					}
					inFile.close();
				}
				else
				{
					std::cerr << "\n!!!ERROR open file: " << file_path << std::endl;
					ret = -1;
					break;
				}
#pragma omp critical 
				{
					all_lines += cnt_lines;
					std::cout << "PROCESSED " << cnt_lines << " ROWS IN FILE " << file_path << "\r";
				}
			}
			else {
#pragma omp critical 
				{
					std::cout << "!!! WORNING !!! COUNT LINES IS 0, FILE " << file_path << std::endl;
				}
			}

		}

		std::cout << "\nALL ADDRESSES IN FILES " << all_lines << std::endl;
		std::cout << "TEMP MALLOC ALL RAM MEMORY SIZE (DATABASE): " << std::to_string((float)(all_lines * 20) / (1024.0f * 1024.0f * 1024.0f)) << " GB\n";
		*num_addresses_in_tables = all_lines;
		return ret;
	}

	void clearFiles() {
		std::ofstream out;
		out.open(FILE_PATH_RESULT);
		out.close();
	}

	void saveResult(char* mnemonic, uint8_t* hash160, size_t num_wallets, size_t num_all_childs) {
		std::ofstream out;
		out.open(FILE_PATH_RESULT, std::ios::app);
#pragma omp parallel for 
		for (int x = 0; x < NUM_PACKETS_SAVE_IN_FILE; x++) {
			if (out.is_open())
			{
				for (int i = x * (int)num_wallets / NUM_PACKETS_SAVE_IN_FILE; i < (x * (int)num_wallets / NUM_PACKETS_SAVE_IN_FILE + (int)num_wallets / NUM_PACKETS_SAVE_IN_FILE); i++) {
					std::string addr;
					std::stringstream ss;
					ss << (const char*)&mnemonic[SIZE_MNEMONIC_FRAME * i];
					for (int ii = 0; ii < num_all_childs; ii++) {
						uint8_t* hash = (uint8_t*)&hash160[(i * num_all_childs + ii) * 20];
						encodeAddressBase58((const uint8_t*)hash, addr);
						ss << ',' << addr;
					}
					ss << '\n';
#pragma omp critical
					{
						out << ss.str();
					}
				}
			}
			else
			{
				printf("\n!!!ERROR create file %s!!!\n", FILE_PATH_RESULT);
			}
		}
		out.close();
	}
	void addFoundMnemonicInFile(std::string path, std::string mnemonic, const char* address) {
		std::ofstream out;
		std::string pth = path;
		out.open(FILE_PATH_FOUND_ADDRESSES, std::ios::app);
		if (out.is_open())
		{
			std::time_t result = std::time(nullptr);
			out << mnemonic << ",address path " << pth << "," << address << "," << std::asctime(std::localtime(&result));
		}
		else
		{
			printf("\n!!!ERROR open file %s!!!\n", FILE_PATH_FOUND_ADDRESSES);
		}
		out.close();
	}

	void addInFileTest(int num_bytes, std::string path, std::string& mnemonic, std::string& hash160, std::string& hash160_in_table, std::string& addr, std::string& addr_in_table) {
		std::ofstream out;
		out.open(FILE_PATH_FOUND_BYTES, std::ios::app);
		if (out.is_open())
		{
			const std::time_t now = std::time(nullptr);
			out << "EQUAL " << num_bytes << "," << mnemonic << ",address path " << path << ":," << addr << "," << "address in table:," << addr_in_table << ",hash160:," << hash160 << ",hash160 in table:," << hash160_in_table << "," << std::asctime(std::localtime(&now));
		}
		else
		{
			printf("\n!!!ERROR open file %s!!!\n", FILE_PATH_FOUND_BYTES);
		}
		out.close();
	}

	std::string getPath(uint32_t path, uint32_t child)
	{
		std::stringstream ss;
		std::string pth = "";
		if (path == 0) ss << "m/0/" << child;
		if (path == 1) ss << "m/1/" << child;
		return ss.str();
	}

	int checkResult(retStruct* ret) {

		if (ret->f[0].count_found >= MAX_FOUND_ADDRESSES)
		{
			ret->f[0].count_found = MAX_FOUND_ADDRESSES;
			std::cout << "\n!!!WARNING ret->f[0].count_found >= MAX_FOUND_ADDRESSES!!!\n";
			std::cout << "!!!PLEASE INCREASE MAX_FOUND_ADDRESSES" << std::endl;
			std::cout << "!!!WARNING ret->f[0].count_found >= MAX_FOUND_ADDRESSES!!!\n";

		}
		if (ret->f[0].count_found_bytes >= MAX_FOUND_ADDRESSES)
		{
			ret->f[0].count_found_bytes = MAX_FOUND_ADDRESSES;
			std::cout << "\n!!!WARNING ret->f[0].count_found_bytes >= MAX_FOUND_ADDRESSES!!!\n";
			std::cout << "!!!PLEASE INCREASE MAX_FOUND_ADDRESSES" << std::endl;
			std::cout << "!!!WARNING ret->f[0].count_found_bytes >= MAX_FOUND_ADDRESSES!!!\n";

		}
		if (ret->f[0].count_found != 0)
		{
			for (uint32_t i = 0; i < ret->f[0].count_found; i++)
			{
				foundInfoStruct* info = &ret->f[0].found_info[i];
				std::string mnemonic = wordsIndexToMnemonic(info->words_idx);
				std::string addr;
				std::string path = getPath(info->path, info->child);
				tools::encodeAddressBase58((const uint8_t*)info->hash160, addr);
				tools::addFoundMnemonicInFile(path, mnemonic, addr.c_str());
				std::cout << "!!!FOUND!!!\n!!!FOUND!!!\n!!!FOUND!!!\n!!!FOUND!!!\n";
				std::cout << "!!!FOUND ADDRESS (" << path << "): " << mnemonic << ", " << addr << std::endl;
				std::cout << "!!!FOUND!!!\n!!!FOUND!!!\n!!!FOUND!!!\n!!!FOUND!!!\n";
			}
		}

		if (ret->f[0].count_found_bytes != 0)
		{
			for (uint32_t i = 0; i < ret->f[0].count_found_bytes; i++)
			{
				foundBytesInfoStruct* info = &ret->f[0].found_bytes_info[i];
				int num_bytes = 0;
				for (int i = 0; i < 20; i++)
				{
					if (*(uint8_t*)((uint8_t*)info->hash160 + i) != *(uint8_t*)((uint8_t*)info->hash160_from_table + i)) break;
					num_bytes++;
				}
				std::string hash160 = tools::bytesToHexString((const uint8_t*)info->hash160, 20);
				std::string hash160_in_table = tools::bytesToHexString((const uint8_t*)info->hash160_from_table, 20);
				std::string mnemonic = wordsIndexToMnemonic(info->words_idx);
				std::string addr;
				std::string addr_in_table;
				std::string path = getPath(info->path, info->child);
				tools::encodeAddressBase58((const uint8_t*)info->hash160, addr);
				tools::encodeAddressBase58((const uint8_t*)info->hash160_from_table, addr_in_table);
				std::cout << "\n!!!FOUND IN ADDRESS(HASH160) (" << path << ") EQUAL " << num_bytes << " BYTES: " << mnemonic << "," << addr << "," << addr_in_table << "," << hash160 << "," << hash160_in_table << " \n";
				tools::addInFileTest(num_bytes, path, mnemonic, hash160, hash160_in_table, addr, addr_in_table);
			}
		}
		return 0;
	}
	// 19.06 kB
	static const uint8_t mnemonic_words[1626][13] = { "like", "just", "love", "know", "never", "want", "time", "out", "there", "make", "look", "eye", "down", "only", "think", "heart", "back", "then", "into", "about", "more", "away", "still", "them", "take", "thing", "even", "through", "long", "always", "world", "too", "friend", "tell", "try", "hand", "thought", "over", "here", "other", "need", "smile", "again", "much", "cry", "been", "night", "ever", "little", "said", "end", "some", "those", "around", "mind", "people", "girl", "leave", "dream", "left", "turn", "myself", "give", "nothing", "really", "off", "before", "something", "find", "walk", "wish", "good", "once", "place", "ask", "stop", "keep", "watch", "seem", "everything", "wait", "got", "yet", "made", "remember", "start", "alone", "run", "hope", "maybe", "believe", "body", "hate", "after", "close", "talk", "stand", "own", "each", "hurt", "help", "home", "god", "soul", "new", "many", "two", "inside", "should", "true", "first", "fear", "mean", "better", "play", "another", "gone", "change", "use", "wonder", "someone", "hair", "cold", "open", "best", "any", "behind", "happen", "water", "dark", "laugh", "stay", "forever", "name", "work", "show", "sky", "break", "came", "deep", "door", "put", "black", "together", "upon", "happy", "such", "great", "white", "matter", "fill", "past", "please", "burn", "cause", "enough", "touch", "moment", "soon", "voice", "scream", "anything", "stare", "sound", "red", "everyone", "hide", "kiss", "truth", "death", "beautiful", "mine", "blood", "broken", "very", "pass", "next", "forget", "tree", "wrong", "air", "mother", "understand", "lip", "hit", "wall", "memory", "sleep", "free", "high", "realize", "school", "might", "skin", "sweet", "perfect", "blue", "kill", "breath", "dance", "against", "fly", "between", "grow", "strong", "under", "listen", "bring", "sometimes", "speak", "pull", "person", "become", "family", "begin", "ground", "real", "small", "father", "sure", "feet", "rest", "young", "finally", "land", "across", "today", "different", "guy", "line", "fire", "reason", "reach", "second", "slowly", "write", "eat", "smell", "mouth", "step", "learn", "three", "floor", "promise", "breathe", "darkness", "push", "earth", "guess", "save", "song", "above", "along", "both", "color", "house", "almost", "sorry", "anymore", "brother", "okay", "dear", "game", "fade", "already", "apart", "warm", "beauty", "heard", "notice", "question", "shine", "began", "piece", "whole", "shadow", "secret", "street", "within", "finger", "point", "morning", "whisper", "child", "moon", "green", "story", "glass", "kid", "silence", "since", "soft", "yourself", "empty", "shall", "angel", "answer", "baby", "bright", "dad", "path", "worry", "hour", "drop", "follow", "power", "war", "half", "flow", "heaven", "act", "chance", "fact", "least", "tired", "children", "near", "quite", "afraid", "rise", "sea", "taste", "window", "cover", "nice", "trust", "lot", "sad", "cool", "force", "peace", "return", "blind", "easy", "ready", "roll", "rose", "drive", "held", "music", "beneath", "hang", "mom", "paint", "emotion", "quiet", "clear", "cloud", "few", "pretty", "bird", "outside", "paper", "picture", "front", "rock", "simple", "anyone", "meant", "reality", "road", "sense", "waste", "bit", "leaf", "thank", "happiness", "meet", "men", "smoke", "truly", "decide", "self", "age", "book", "form", "alive", "carry", "escape", "damn", "instead", "able", "ice", "minute", "throw", "catch", "leg", "ring", "course", "goodbye", "lead", "poem", "sick", "corner", "desire", "known", "problem", "remind", "shoulder", "suppose", "toward", "wave", "drink", "jump", "woman", "pretend", "sister", "week", "human", "joy", "crack", "grey", "pray", "surprise", "dry", "knee", "less", "search", "bleed", "caught", "clean", "embrace", "future", "king", "son", "sorrow", "chest", "hug", "remain", "sat", "worth", "blow", "daddy", "final", "parent", "tight", "also", "create", "lonely", "safe", "cross", "dress", "evil", "silent", "bone", "fate", "perhaps", "anger", "class", "scar", "snow", "tiny", "tonight", "continue", "control", "dog", "edge", "mirror", "month", "suddenly", "comfort", "given", "loud", "quickly", "gaze", "plan", "rush", "stone", "town", "battle", "ignore", "spirit", "stood", "stupid", "yours", "brown", "build", "dust", "hey", "kept", "pay", "phone", "twist", "although", "ball", "beyond", "hidden", "nose", "taken", "fail", "float", "pure", "somehow", "wash", "wrap", "angry", "cheek", "creature", "forgotten", "heat", "rip", "single", "space", "special", "weak", "whatever", "yell", "anyway", "blame", "job", "choose", "country", "curse", "drift", "echo", "figure", "grew", "laughter", "neck", "suffer", "worse", "yeah", "disappear", "foot", "forward", "knife", "mess", "somewhere", "stomach", "storm", "beg", "idea", "lift", "offer", "breeze", "field", "five", "often", "simply", "stuck", "win", "allow", "confuse", "enjoy", "except", "flower", "seek", "strength", "calm", "grin", "gun", "heavy", "hill", "large", "ocean", "shoe", "sigh", "straight", "summer", "tongue", "accept", "crazy", "everyday", "exist", "grass", "mistake", "sent", "shut", "surround", "table", "ache", "brain", "destroy", "heal", "nature", "shout", "sign", "stain", "choice", "doubt", "glance", "glow", "mountain", "queen", "stranger", "throat", "tomorrow", "city", "either", "fish", "flame", "rather", "shape", "spin", "spread", "ash", "distance", "finish", "image", "imagine", "important", "nobody", "shatter", "warmth", "became", "feed", "flesh", "funny", "lust", "shirt", "trouble", "yellow", "attention", "bare", "bite", "money", "protect", "amaze", "appear", "born", "choke", "completely", "daughter", "fresh", "friendship", "gentle", "probably", "six", "deserve", "expect", "grab", "middle", "nightmare", "river", "thousand", "weight", "worst", "wound", "barely", "bottle", "cream", "regret", "relationship", "stick", "test", "crush", "endless", "fault", "itself", "rule", "spill", "art", "circle", "join", "kick", "mask", "master", "passion", "quick", "raise", "smooth", "unless", "wander", "actually", "broke", "chair", "deal", "favorite", "gift", "note", "number", "sweat", "box", "chill", "clothes", "lady", "mark", "park", "poor", "sadness", "tie", "animal", "belong", "brush", "consume", "dawn", "forest", "innocent", "pen", "pride", "stream", "thick", "clay", "complete", "count", "draw", "faith", "press", "silver", "struggle", "surface", "taught", "teach", "wet", "bless", "chase", "climb", "enter", "letter", "melt", "metal", "movie", "stretch", "swing", "vision", "wife", "beside", "crash", "forgot", "guide", "haunt", "joke", "knock", "plant", "pour", "prove", "reveal", "steal", "stuff", "trip", "wood", "wrist", "bother", "bottom", "crawl", "crowd", "fix", "forgive", "frown", "grace", "loose", "lucky", "party", "release", "surely", "survive", "teacher", "gently", "grip", "speed", "suicide", "travel", "treat", "vein", "written", "cage", "chain", "conversation", "date", "enemy", "however", "interest", "million", "page", "pink", "proud", "sway", "themselves", "winter", "church", "cruel", "cup", "demon", "experience", "freedom", "pair", "pop", "purpose", "respect", "shoot", "softly", "state", "strange", "bar", "birth", "curl", "dirt", "excuse", "lord", "lovely", "monster", "order", "pack", "pants", "pool", "scene", "seven", "shame", "slide", "ugly", "among", "blade", "blonde", "closet", "creek", "deny", "drug", "eternity", "gain", "grade", "handle", "key", "linger", "pale", "prepare", "swallow", "swim", "tremble", "wheel", "won", "cast", "cigarette", "claim", "college", "direction", "dirty", "gather", "ghost", "hundred", "loss", "lung", "orange", "present", "swear", "swirl", "twice", "wild", "bitter", "blanket", "doctor", "everywhere", "flash", "grown", "knowledge", "numb", "pressure", "radio", "repeat", "ruin", "spend", "unknown", "buy", "clock", "devil", "early", "false", "fantasy", "pound", "precious", "refuse", "sheet", "teeth", "welcome", "add", "ahead", "block", "bury", "caress", "content", "depth", "despite", "distant", "marry", "purple", "threw", "whenever", "bomb", "dull", "easily", "grasp", "hospital", "innocence", "normal", "receive", "reply", "rhyme", "shade", "someday", "sword", "toe", "visit", "asleep", "bought", "center", "consider", "flat", "hero", "history", "ink", "insane", "muscle", "mystery", "pocket", "reflection", "shove", "silently", "smart", "soldier", "spot", "stress", "train", "type", "view", "whether", "bus", "energy", "explain", "holy", "hunger", "inch", "magic", "mix", "noise", "nowhere", "prayer", "presence", "shock", "snap", "spider", "study", "thunder", "trail", "admit", "agree", "bag", "bang", "bound", "butterfly", "cute", "exactly", "explode", "familiar", "fold", "further", "pierce", "reflect", "scent", "selfish", "sharp", "sink", "spring", "stumble", "universe", "weep", "women", "wonderful", "action", "ancient", "attempt", "avoid", "birthday", "branch", "chocolate", "core", "depress", "drunk", "especially", "focus", "fruit", "honest", "match", "palm", "perfectly", "pillow", "pity", "poison", "roar", "shift", "slightly", "thump", "truck", "tune", "twenty", "unable", "wipe", "wrote", "coat", "constant", "dinner", "drove", "egg", "eternal", "flight", "flood", "frame", "freak", "gasp", "glad", "hollow", "motion", "peer", "plastic", "root", "screen", "season", "sting", "strike", "team", "unlike", "victim", "volume", "warn", "weird", "attack", "await", "awake", "built", "charm", "crave", "despair", "fought", "grant", "grief", "horse", "limit", "message", "ripple", "sanity", "scatter", "serve", "split", "string", "trick", "annoy", "blur", "boat", "brave", "clearly", "cling", "connect", "fist", "forth", "imagination", "iron", "jock", "judge", "lesson", "milk", "misery", "nail", "naked", "ourselves", "poet", "possible", "princess", "sail", "size", "snake", "society", "stroke", "torture", "toss", "trace", "wise", "bloom", "bullet", "cell", "check", "cost", "darling", "during", "footstep", "fragile", "hallway", "hardly", "horizon", "invisible", "journey", "midnight", "mud", "nod", "pause", "relax", "shiver", "sudden", "value", "youth", "abuse", "admire", "blink", "breast", "bruise", "constantly", "couple", "creep", "curve", "difference", "dumb", "emptiness", "gotta", "honor", "plain", "planet", "recall", "rub", "ship", "slam", "soar", "somebody", "tightly", "weather", "adore", "approach", "bond", "bread", "burst", "candle", "coffee", "cousin", "crime", "desert", "flutter", "frozen", "grand", "heel", "hello", "language", "level", "movement", "pleasure", "powerful", "random", "rhythm", "settle", "silly", "slap", "sort", "spoken", "steel", "threaten", "tumble", "upset", "aside", "awkward", "bee", "blank", "board", "button", "card", "carefully", "complain", "crap", "deeply", "discover", "drag", "dread", "effort", "entire", "fairy", "giant", "gotten", "greet", "illusion", "jeans", "leap", "liquid", "march", "mend", "nervous", "nine", "replace", "rope", "spine", "stole", "terror", "accident", "apple", "balance", "boom", "childhood", "collect", "demand", "depression", "eventually", "faint", "glare", "goal", "group", "honey", "kitchen", "laid", "limb", "machine", "mere", "mold", "murder", "nerve", "painful", "poetry", "prince", "rabbit", "shelter", "shore", "shower", "soothe", "stair", "steady", "sunlight", "tangle", "tease", "treasure", "uncle", "begun", "bliss", "canvas", "cheer", "claw", "clutch", "commit", "crimson", "crystal", "delight", "doll", "existence", "express", "fog", "football", "gay", "goose", "guard", "hatred", "illuminate", "mass", "math", "mourn", "rich", "rough", "skip", "stir", "student", "style", "support", "thorn", "tough", "yard", "yearn", "yesterday", "advice", "appreciate", "autumn", "bank", "beam", "bowl", "capture", "carve", "collapse", "confusion", "creation", "dove", "feather", "girlfriend", "glory", "government", "harsh", "hop", "inner", "loser", "moonlight", "neighbor", "neither", "peach", "pig", "praise", "screw", "shield", "shimmer", "sneak", "stab", "subject", "throughout", "thrown", "tower", "twirl", "wow", "army", "arrive", "bathroom", "bump", "cease", "cookie", "couch", "courage", "dim", "guilt", "howl", "hum", "husband", "insult", "led", "lunch", "mock", "mostly", "natural", "nearly", "needle", "nerd", "peaceful", "perfection", "pile", "price", "remove", "roam", "sanctuary", "serious", "shiny", "shook", "sob", "stolen", "tap", "vain", "void", "warrior", "wrinkle", "affection", "apologize", "blossom", "bounce", "bridge", "cheap", "crumble", "decision", "descend", "desperately", "dig", "dot", "flip", "frighten", "heartbeat", "huge", "lazy", "lick", "odd", "opinion", "process", "puzzle", "quietly", "retreat", "score", "sentence", "separate", "situation", "skill", "soak", "square", "stray", "taint", "task", "tide", "underneath", "veil", "whistle", "anywhere", "bedroom", "bid", "bloody", "burden", "careful", "compare", "concern", "curtain", "decay", "defeat", "describe", "double", "dreamer", "driver", "dwell", "evening", "flare", "flicker", "grandma", "guitar", "harm", "horrible", "hungry", "indeed", "lace", "melody", "monkey", "nation", "object", "obviously", "rainbow", "salt", "scratch", "shown", "shy", "stage", "stun", "third", "tickle", "useless", "weakness", "worship", "worthless", "afternoon", "beard", "boyfriend", "bubble", "busy", "certain", "chin", "concrete", "desk", "diamond", "doom", "drawn", "due", "felicity", "freeze", "frost", "garden", "glide", "harmony", "hopefully", "hunt", "jealous", "lightning", "mama", "mercy", "peel", "physical", "position", "pulse", "punch", "quit", "rant", "respond", "salty", "sane", "satisfy", "savior", "sheep", "slept", "social", "sport", "tuck", "utter", "valley", "wolf", "aim", "alas", "alter", "arrow", "awaken", "beaten", "belief", "brand", "ceiling", "cheese", "clue", "confidence", "connection", "daily", "disguise", "eager", "erase", "essence", "everytime", "expression", "fan", "flag", "flirt", "foul", "fur", "giggle", "glorious", "ignorance", "law", "lifeless", "measure", "mighty", "muse", "north", "opposite", "paradise", "patience", "patient", "pencil", "petal", "plate", "ponder", "possibly", "practice", "slice", "spell", "stock", "strife", "strip", "suffocate", "suit", "tender", "tool", "trade", "velvet", "verse", "waist", "witch", "aunt", "bench", "bold", "cap", "certainly", "click", "companion", "creator", "dart", "delicate", "determine", "dish", "dragon", "drama", "drum", "dude", "everybody", "feast", "forehead", "former", "fright", "fully", "gas", "hook", "hurl", "invite", "juice", "manage", "moral", "possess", "raw", "rebel", "royal", "scale", "scary", "several", "slight", "stubborn", "swell", "talent", "tea", "terrible", "thread", "torment", "trickle", "usually", "vast", "violence", "weave", "acid", "agony", "ashamed", "awe", "belly", "blend", "blush", "character", "cheat", "common", "company", "coward", "creak", "danger", "deadly", "defense", "define", "depend", "desperate", "destination", "dew", "duck", "dusty", "embarrass", "engine", "example", "explore", "foe", "freely", "frustrate", "generation", "glove", "guilty", "health", "hurry", "idiot", "impossible", "inhale", "jaw", "kingdom", "mention", "mist", "moan", "mumble", "mutter", "observe", "ode", "pathetic", "pattern", "pie", "prefer", "puff", "rape", "rare", "revenge", "rude", "scrape", "spiral", "squeeze", "strain", "sunset", "suspend", "sympathy", "thigh", "throne", "total", "unseen", "weapon", "weary" };
	// 1626 B
	static const uint8_t mnemonic_word_lengths[1626] = { 4, 4, 4, 4, 5, 4, 4, 3, 5, 4, 4, 3, 4, 4, 5, 5, 4, 4, 4, 5, 4, 4, 5, 4, 4, 5, 4, 7, 4, 6, 5, 3, 6, 4, 3, 4, 7, 4, 4, 5, 4, 5, 5, 4, 3, 4, 5, 4, 6, 4, 3, 4, 5, 6, 4, 6, 4, 5, 5, 4, 4, 6, 4, 7, 6, 3, 6, 9, 4, 4, 4, 4, 4, 5, 3, 4, 4, 5, 4, 10, 4, 3, 3, 4, 8, 5, 5, 3, 4, 5, 7, 4, 4, 5, 5, 4, 5, 3, 4, 4, 4, 4, 3, 4, 3, 4, 3, 6, 6, 4, 5, 4, 4, 6, 4, 7, 4, 6, 3, 6, 7, 4, 4, 4, 4, 3, 6, 6, 5, 4, 5, 4, 7, 4, 4, 4, 3, 5, 4, 4, 4, 3, 5, 8, 4, 5, 4, 5, 5, 6, 4, 4, 6, 4, 5, 6, 5, 6, 4, 5, 6, 8, 5, 5, 3, 8, 4, 4, 5, 5, 9, 4, 5, 6, 4, 4, 4, 6, 4, 5, 3, 6, 10, 3, 3, 4, 6, 5, 4, 4, 7, 6, 5, 4, 5, 7, 4, 4, 6, 5, 7, 3, 7, 4, 6, 5, 6, 5, 9, 5, 4, 6, 6, 6, 5, 6, 4, 5, 6, 4, 4, 4, 5, 7, 4, 6, 5, 9, 3, 4, 4, 6, 5, 6, 6, 5, 3, 5, 5, 4, 5, 5, 5, 7, 7, 8, 4, 5, 5, 4, 4, 5, 5, 4, 5, 5, 6, 5, 7, 7, 4, 4, 4, 4, 7, 5, 4, 6, 5, 6, 8, 5, 5, 5, 5, 6, 6, 6, 6, 6, 5, 7, 7, 5, 4, 5, 5, 5, 3, 7, 5, 4, 8, 5, 5, 5, 6, 4, 6, 3, 4, 5, 4, 4, 6, 5, 3, 4, 4, 6, 3, 6, 4, 5, 5, 8, 4, 5, 6, 4, 3, 5, 6, 5, 4, 5, 3, 3, 4, 5, 5, 6, 5, 4, 5, 4, 4, 5, 4, 5, 7, 4, 3, 5, 7, 5, 5, 5, 3, 6, 4, 7, 5, 7, 5, 4, 6, 6, 5, 7, 4, 5, 5, 3, 4, 5, 9, 4, 3, 5, 5, 6, 4, 3, 4, 4, 5, 5, 6, 4, 7, 4, 3, 6, 5, 5, 3, 4, 6, 7, 4, 4, 4, 6, 6, 5, 7, 6, 8, 7, 6, 4, 5, 4, 5, 7, 6, 4, 5, 3, 5, 4, 4, 8, 3, 4, 4, 6, 5, 6, 5, 7, 6, 4, 3, 6, 5, 3, 6, 3, 5, 4, 5, 5, 6, 5, 4, 6, 6, 4, 5, 5, 4, 6, 4, 4, 7, 5, 5, 4, 4, 4, 7, 8, 7, 3, 4, 6, 5, 8, 7, 5, 4, 7, 4, 4, 4, 5, 4, 6, 6, 6, 5, 6, 5, 5, 5, 4, 3, 4, 3, 5, 5, 8, 4, 6, 6, 4, 5, 4, 5, 4, 7, 4, 4, 5, 5, 8, 9, 4, 3, 6, 5, 7, 4, 8, 4, 6, 5, 3, 6, 7, 5, 5, 4, 6, 4, 8, 4, 6, 5, 4, 9, 4, 7, 5, 4, 9, 7, 5, 3, 4, 4, 5, 6, 5, 4, 5, 6, 5, 3, 5, 7, 5, 6, 6, 4, 8, 4, 4, 3, 5, 4, 5, 5, 4, 4, 8, 6, 6, 6, 5, 8, 5, 5, 7, 4, 4, 8, 5, 4, 5, 7, 4, 6, 5, 4, 5, 6, 5, 6, 4, 8, 5, 8, 6, 8, 4, 6, 4, 5, 6, 5, 4, 6, 3, 8, 6, 5, 7, 9, 6, 7, 6, 6, 4, 5, 5, 4, 5, 7, 6, 9, 4, 4, 5, 7, 5, 6, 4, 5, 10, 8, 5, 10, 6, 8, 3, 7, 6, 4, 6, 9, 5, 8, 6, 5, 5, 6, 6, 5, 6, 12, 5, 4, 5, 7, 5, 6, 4, 5, 3, 6, 4, 4, 4, 6, 7, 5, 5, 6, 6, 6, 8, 5, 5, 4, 8, 4, 4, 6, 5, 3, 5, 7, 4, 4, 4, 4, 7, 3, 6, 6, 5, 7, 4, 6, 8, 3, 5, 6, 5, 4, 8, 5, 4, 5, 5, 6, 8, 7, 6, 5, 3, 5, 5, 5, 5, 6, 4, 5, 5, 7, 5, 6, 4, 6, 5, 6, 5, 5, 4, 5, 5, 4, 5, 6, 5, 5, 4, 4, 5, 6, 6, 5, 5, 3, 7, 5, 5, 5, 5, 5, 7, 6, 7, 7, 6, 4, 5, 7, 6, 5, 4, 7, 4, 5, 12, 4, 5, 7, 8, 7, 4, 4, 5, 4, 10, 6, 6, 5, 3, 5, 10, 7, 4, 3, 7, 7, 5, 6, 5, 7, 3, 5, 4, 4, 6, 4, 6, 7, 5, 4, 5, 4, 5, 5, 5, 5, 4, 5, 5, 6, 6, 5, 4, 4, 8, 4, 5, 6, 3, 6, 4, 7, 7, 4, 7, 5, 3, 4, 9, 5, 7, 9, 5, 6, 5, 7, 4, 4, 6, 7, 5, 5, 5, 4, 6, 7, 6, 10, 5, 5, 9, 4, 8, 5, 6, 4, 5, 7, 3, 5, 5, 5, 5, 7, 5, 8, 6, 5, 5, 7, 3, 5, 5, 4, 6, 7, 5, 7, 7, 5, 6, 5, 8, 4, 4, 6, 5, 8, 9, 6, 7, 5, 5, 5, 7, 5, 3, 5, 6, 6, 6, 8, 4, 4, 7, 3, 6, 6, 7, 6, 10, 5, 8, 5, 7, 4, 6, 5, 4, 4, 7, 3, 6, 7, 4, 6, 4, 5, 3, 5, 7, 6, 8, 5, 4, 6, 5, 7, 5, 5, 5, 3, 4, 5, 9, 4, 7, 7, 8, 4, 7, 6, 7, 5, 7, 5, 4, 6, 7, 8, 4, 5, 9, 6, 7, 7, 5, 8, 6, 9, 4, 7, 5, 10, 5, 5, 6, 5, 4, 9, 6, 4, 6, 4, 5, 8, 5, 5, 4, 6, 6, 4, 5, 4, 8, 6, 5, 3, 7, 6, 5, 5, 5, 4, 4, 6, 6, 4, 7, 4, 6, 6, 5, 6, 4, 6, 6, 6, 4, 5, 6, 5, 5, 5, 5, 5, 7, 6, 5, 5, 5, 5, 7, 6, 6, 7, 5, 5, 6, 5, 5, 4, 4, 5, 7, 5, 7, 4, 5, 11, 4, 4, 5, 6, 4, 6, 4, 5, 9, 4, 8, 8, 4, 4, 5, 7, 6, 7, 4, 5, 4, 5, 6, 4, 5, 4, 7, 6, 8, 7, 7, 6, 7, 9, 7, 8, 3, 3, 5, 5, 6, 6, 5, 5, 5, 6, 5, 6, 6, 10, 6, 5, 5, 10, 4, 9, 5, 5, 5, 6, 6, 3, 4, 4, 4, 8, 7, 7, 5, 8, 4, 5, 5, 6, 6, 6, 5, 6, 7, 6, 5, 4, 5, 8, 5, 8, 8, 8, 6, 6, 6, 5, 4, 4, 6, 5, 8, 6, 5, 5, 7, 3, 5, 5, 6, 4, 9, 8, 4, 6, 8, 4, 5, 6, 6, 5, 5, 6, 5, 8, 5, 4, 6, 5, 4, 7, 4, 7, 4, 5, 5, 6, 8, 5, 7, 4, 9, 7, 6, 10, 10, 5, 5, 4, 5, 5, 7, 4, 4, 7, 4, 4, 6, 5, 7, 6, 6, 6, 7, 5, 6, 6, 5, 6, 8, 6, 5, 8, 5, 5, 5, 6, 5, 4, 6, 6, 7, 7, 7, 4, 9, 7, 3, 8, 3, 5, 5, 6, 10, 4, 4, 5, 4, 5, 4, 4, 7, 5, 7, 5, 5, 4, 5, 9, 6, 10, 6, 4, 4, 4, 7, 5, 8, 9, 8, 4, 7, 10, 5, 10, 5, 3, 5, 5, 9, 8, 7, 5, 3, 6, 5, 6, 7, 5, 4, 7, 10, 6, 5, 5, 3, 4, 6, 8, 4, 5, 6, 5, 7, 3, 5, 4, 3, 7, 6, 3, 5, 4, 6, 7, 6, 6, 4, 8, 10, 4, 5, 6, 4, 9, 7, 5, 5, 3, 6, 3, 4, 4, 7, 7, 9, 9, 7, 6, 6, 5, 7, 8, 7, 11, 3, 3, 4, 8, 9, 4, 4, 4, 3, 7, 7, 6, 7, 7, 5, 8, 8, 9, 5, 4, 6, 5, 5, 4, 4, 10, 4, 7, 8, 7, 3, 6, 6, 7, 7, 7, 7, 5, 6, 8, 6, 7, 6, 5, 7, 5, 7, 7, 6, 4, 8, 6, 6, 4, 6, 6, 6, 6, 9, 7, 4, 7, 5, 3, 5, 4, 5, 6, 7, 8, 7, 9, 9, 5, 9, 6, 4, 7, 4, 8, 4, 7, 4, 5, 3, 8, 6, 5, 6, 5, 7, 9, 4, 7, 9, 4, 5, 4, 8, 8, 5, 5, 4, 4, 7, 5, 4, 7, 6, 5, 5, 6, 5, 4, 5, 6, 4, 3, 4, 5, 5, 6, 6, 6, 5, 7, 6, 4, 10, 10, 5, 8, 5, 5, 7, 9, 10, 3, 4, 5, 4, 3, 6, 8, 9, 3, 8, 7, 6, 4, 5, 8, 8, 8, 7, 6, 5, 5, 6, 8, 8, 5, 5, 5, 6, 5, 9, 4, 6, 4, 5, 6, 5, 5, 5, 4, 5, 4, 3, 9, 5, 9, 7, 4, 8, 9, 4, 6, 5, 4, 4, 9, 5, 8, 6, 6, 5, 3, 4, 4, 6, 5, 6, 5, 7, 3, 5, 5, 5, 5, 7, 6, 8, 5, 6, 3, 8, 6, 7, 7, 7, 4, 8, 5, 4, 5, 7, 3, 5, 5, 5, 9, 5, 6, 7, 6, 5, 6, 6, 7, 6, 6, 9, 11, 3, 4, 5, 9, 6, 7, 7, 3, 6, 9, 10, 5, 6, 6, 5, 5, 10, 6, 3, 7, 7, 4, 4, 6, 6, 7, 3, 8, 7, 3, 6, 4, 4, 4, 7, 4, 6, 6, 7, 6, 6, 7, 8, 5, 6, 5, 6, 6, 5 };


	int stringToWordIndices(std::string str, int16_t* gen_words_indices) {

		std::stringstream X(str);
		std::string word;
		std::vector<std::string> words;
		while (getline(X, word, ' ')) {
			words.push_back(word);
		}

		if (words.size() != NUM_WORDS_MNEMONIC)
		{
			std::cerr << "!!!ERROR PARSE STRING: \"" << str << "\"" << std::endl;
			return -1;
		}

		for (int i = 0; i < NUM_WORDS_MNEMONIC; i++)
		{
			bool found = false;
			if (strcmp(words[i].c_str(), (const char*)"?") == 0)
			{
				gen_words_indices[i] = -1;
				continue;
			}
			for (int ii = 0; ii < 1626; ii++)
			{
				if (strcmp(words[i].c_str(), (const char*)mnemonic_words[ii]) == 0)
				{
					gen_words_indices[i] = ii;
					found = true;
					break;
				}
			}
			if (!found) {
				std::cerr << "!!!ERROR PARSE STRING: \"" << str << "\"" << std::endl;
				std::cerr << "!!!WRONG WORD " << i << ": \"" << words[i] << "\"" << std::endl;
				return -1;
			}
		}

		return 0;
	}
}
