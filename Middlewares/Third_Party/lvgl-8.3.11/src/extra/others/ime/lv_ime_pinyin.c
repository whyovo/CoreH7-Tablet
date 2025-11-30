/**
 * @file lv_ime_pinyin.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_ime_pinyin.h"
#if LV_USE_IME_PINYIN != 0

#include <stdio.h>

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS    &lv_ime_pinyin_class

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_ime_pinyin_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_ime_pinyin_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_ime_pinyin_style_change_event(lv_event_t * e);
static void lv_ime_pinyin_kb_event(lv_event_t * e);
static void lv_ime_pinyin_cand_panel_event(lv_event_t * e);

static void init_pinyin_dict(lv_obj_t * obj, lv_pinyin_dict_t * dict);
static void pinyin_input_proc(lv_obj_t * obj);
static void pinyin_page_proc(lv_obj_t * obj, uint16_t btn);
static char * pinyin_search_matching(lv_obj_t * obj, char * py_str, uint16_t * cand_num);
static void pinyin_ime_clear_data(lv_obj_t * obj);

#if LV_IME_PINYIN_USE_K9_MODE
    static void pinyin_k9_init_data(lv_obj_t * obj);
    static void pinyin_k9_get_legal_py(lv_obj_t * obj, char * k9_input, const char * py9_map[]);
    static bool pinyin_k9_is_valid_py(lv_obj_t * obj, char * py_str);
    static void pinyin_k9_fill_cand(lv_obj_t * obj);
    static void pinyin_k9_cand_page_proc(lv_obj_t * obj, uint16_t dir);
#endif

/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t lv_ime_pinyin_class = {
    .constructor_cb = lv_ime_pinyin_constructor,
    .destructor_cb  = lv_ime_pinyin_destructor,
    .width_def      = LV_SIZE_CONTENT,
    .height_def     = LV_SIZE_CONTENT,
    .group_def      = LV_OBJ_CLASS_GROUP_DEF_TRUE,
    .instance_size  = sizeof(lv_ime_pinyin_t),
    .base_class     = &lv_obj_class
};

#if LV_IME_PINYIN_USE_K9_MODE
static char * lv_btnm_def_pinyin_k9_map[LV_IME_PINYIN_K9_CAND_TEXT_NUM + 20] = {\
                                                                                ",\0", "1#\0",  "abc \0", "def\0",  LV_SYMBOL_BACKSPACE"\0", "\n\0",
                                                                                ".\0", "ghi\0", "jkl\0", "mno\0",  LV_SYMBOL_KEYBOARD"\0", "\n\0",
                                                                                "?\0", "pqrs\0", "tuv\0", "wxyz\0",  LV_SYMBOL_NEW_LINE"\0", "\n\0",
                                                                                LV_SYMBOL_LEFT"\0", "\0"
                                                                               };

static lv_btnmatrix_ctrl_t default_kb_ctrl_k9_map[LV_IME_PINYIN_K9_CAND_TEXT_NUM + 16] = { 1 };
static char   lv_pinyin_k9_cand_str[LV_IME_PINYIN_K9_CAND_TEXT_NUM + 2][LV_IME_PINYIN_K9_MAX_INPUT] = {0};
#endif

static char   lv_pinyin_cand_str[LV_IME_PINYIN_CAND_TEXT_NUM][4];
static char * lv_btnm_def_pinyin_sel_map[LV_IME_PINYIN_CAND_TEXT_NUM + 3];

#if LV_IME_PINYIN_USE_DEFAULT_DICT
lv_pinyin_dict_t lv_ime_pinyin_def_dict[] = {
    {"a", "吖呵啊嗄腌锕阿"},
    {"ai", "爱乃呃呆哀哎唉嗌嗳噫埃奇嫒挨捱暧瑷癌皑矮砹碍艾蔼锿隘霭"},
    {"an", "俺厂埯安岸干广庵按揞暗案桉氨犴盒胺谙铵鞍鹌黯"},
    {"ang", "仰昂盎肮腌"},
    {"ao", "傲凹嗷噢嚣坳奥媪岙廒懊拗敖棍澳熬燠獒翱聱螯袄遨鏊鏖骜鳌"},
    {"ba", "把伯八叭吧坝岜巴扒拔捌捭杷湃灞爸疤笆粑罢耙芭茇菝萆跋钯霸靶魃鲅"},
    {"bai", "派伯佰呗啡扒拜捭排掰摆擘柏白百稗薜败鞴"},
    {"ban", "伴办半卑坂彬扮扳拌搬斑板版班瓣瘢癍绊舨般豳辨钣阪颁"},
    {"bang", "傍帮彭旁梆棒榜浜磅绑膀蒡蚌螃谤邦镑"},
    {"bao",
     "保刨剥勹包呆堡孢宝报抱暴曝瀑炮煲爆胞苞苴葆薄袍裒褒褓豹趵雹饱鲍鸨龅"},
    {"bei", "被俾倍北卑呗埤备孛怫悖悲惫拔杯波焙狈碑碚背臂菩萆葡蓓蜚褙贝跋辈邶鐾"
            "钡陂鞴鹎"},
    {"ben", "本体坌夯奔畚笨苯贲锛"},
    {"beng", "平俸傍唪嘣堋崩抨旁榜泵甏甭绷蚌蹦迸"},
    {"bi", "被比仳佛俾匕卑吡哔埤壁妣婢媲嬖币幅庇庳弊弼彼必愎拂捭敝服枇檗殍毕毖"
           "毙泌波滗濞狴璧畀痹瞥碧秕秘笔筚箅篦肥脾臂舭芘荜荸萆蓖蔽薜虑裨襞贲跛"
           "跸辟逼避鄙铋闭陂陛陴馥髀鼻"},
    {"bian", "便变匾卞封弁忭扁拚汴煸砭碥稹窆笾缏编苄蝙褊贬辨辩辫边遍鞭鳊"},
    {"biao", "剽婊嫖彪杓标漂灬瘭膘苞表裱鏖镖镳飑飙飚骠髟鳔"},
    {"bie", "别憋扒拔捌撇瘪秘蔽蹩鳖"},
    {"bin", "份傧宾彬摈斌槟殡浜滨濒玢缤膑豳镔髌鬓"},
    {"bing", "并平丙兵冫冰屏拼摒枋柄槟炳燹病禀秉邴饼"},
    {"bo", "亳伯佛剥勃募博卜啵孛帛怕拍拔拨搏播擗擘暴服柏檗泊波渤溥潘瀑爆玻番白"
           "百礴箔簸簿脖膊般舶艴菠菩蒲蕃薄薜蘖趵跑跛踣钵钹铂饽驳魄鲅鹁"},
    {"bu", "不部卜卟哺埔埠堡布怖拊捕晡步溥瓿簿薄补逋醭钚钸附鞴"},
    {"ca", "嚓拆擦礤蔡"},
    {"cai", "才彩材猜睬菜蔡裁财踩采"},
    {"can", "参孱惨惭掺残灿璨粲蚕餐骖黪"},
    {"cang", "仓伧沧臧舱苍藏"},
    {"cao", "嘈屮操曹槽漕澡糙艚艹草螬造"},
    {"ce", "侧册厕恻栅测策赦"},
    {"cen", "参岑涔"},
    {"ceng", "僧噌增层曾蹭"},
    {"cha", "接刹叉喳嚓土姹察岔差捷插搽斜杈查楂槎檫汊猹碴苴茬茶荼衩诧锸镲馇"},
    {"chai", "侪差拆搓查柴瘥茈虿豺钗"},
    {"chan",
     "产兔冁单厘婵嬗孱廛忏掺搀沾潺澶禅缠羼苫蒇蝉蟾觇谄谗躔铲镡阐颤馋骣"},
    {"chang", "常长伥倘倡偿厂唱场娼嫦尚尝徜怅惝敞昌昶氅淌猖畅肠苌菖裳阊鬯鲳"},
    {"chao", "剿吵嘲巢怊抄晁朝潮炒焯绰耖超钞"},
    {"che", "车坼宅尺屮彻扯拆掣撤斥池澈砗"},
    {"chen",
     "伧嗔堪填宸尘帘忱抻晨枕榇橙沈沉湛琛疹眈碜称肜胂臣衬谌谶趁辰郴陈龀"},
    {"cheng", "成丞乘净呈噌城埕塍嵊徵惩承撑敞晟枨柽棱樘橙澄盛盯瞠秤称程蛏裎诚趟"
              "逞郢酲醒铖铛骋"},
    {"chi", "她侈傺匙叱吃哆哧啻喜嗤坻墀媸尺弛彳抬拆拖持提搋敕斥柢池沱治炽痴瘛眙"
            "眵离移笞篪翅耻胝芪茌茬莉蚩蛇螭褫豉赤踅踟迟郗饬驰魑鸱齿"},
    {"chong", "种重僮充冲宠崇忡憧涌潼烛盅舂艟茧茺虫酮铳"},
    {"chou", "丑仇俦圳妯帱惆愁扭抽揄溴畴瘳瞅稠筹绸臭踌酬雠"},
    {"chu", "出亍储刍初助厨处怵憷搐杵柠楚楮樗橱涂淑滁畜矗础硫祝絮绌著蜍褚触蹰躇"
            "锄除雏黜"},
    {"chua", "撮"},
    {"chuai", "啐啜嘬揣搋膪踹"},
    {"chuan", "串传喘巛川惴掾椽氚穿舛舡船踹遄钏"},
    {"chuang", "创囱幢床怆疮窗舂葱闯"},
    {"chui", "吹垂捶棰椎槌炊锤陲"},
    {"chun", "唇春朐椿沌淳纯肫莼蝽蠢醇鹑"},
    {"chuo", "促啜戳斫淖焯簇绰荃蔟趵踔踱躇辍辶醛龊"},
    {"ci", "次此伺兹刺司呲嵯差慈措柴滋瓷疵磁祠粢糍茈茨蚝螅词赐趑辞雌鹚"},
    {"cong", "从丛偬匆囱枞淙琮璁窗聪苁葱骢"},
    {"cou", "凑奏揍族楱簇腠蔟趣辏"},
    {"cu", "且促卒徂戚槭殂猝簇粗蔟趣蹙蹴酢醋"},
    {"cuan", "撺攒汆爨窜篡蹲蹿镩"},
    {"cui", "体催卒啐察崔悴摧榱毳洒淬璀瘁粹翠脆萃衰隹"},
    {"cun", "存寸忖村浚皴蹲"},
    {"cuo", "最厝嵯差挫措搓摧撮昔痤瘥矬磋脞蹉锉错鹾"},
    {"da", "大打哒嗒塌塔妲怛搭沓疸瘩笪答耷胆褡达迭靼鞑"},
    {"dai", "大代傣呆呔埭岱带待怠戴棣歹殆毒玳甙绐袋贷迨逮逯隶骀黛"},
    {"dan", "但丹儋冉单啖弹忱怛惮憾担掸旦檐殚氮淡湛潭澶澹疸瘅眈石箪耽聃胆膻萏蛋"
            "蜒詹诞赕郸"},
    {"dang", "当党凼宕挡档砀荡菪裆谠铛"},
    {"dao", "到道佻倒刀刂受叨啁导岛帱忉忑悼惆捣敦氘洮焘盗祷稻纛蹈陶"},
    {"de", "的地得底德登锝陟"},
    {"dei", "得"},
    {"deng", "等凳噔嶝戥橙澄灯登瞪磴簦蹬邓镫"},
    {"di", "的地第低勺啻嘀坻堤娣嫡帝底弟抵提敌杓柢棣氐涤滴狄睇砥碲笛籴缔羝翟胝"
           "芍荻莜蒂觌诋谛蹄迪逐递逮邸镝隶骶"},
    {"dia", "嗲"},
    {"dian", "点佃典坫垫埝奠巅店惦拈掂殿沾涎淀滇玷电甸癜癫碘簟蜓踮钿阽靛颠"},
    {"diao", "佻倜凋刀刁叼吊啁挑掉敦碉稠莜蜩调貂跳踔钓铞铫雕鲷鸟"},
    {"die", "佚叠哆喋嗲垤堞揲涉渫爹牒瓞碟窒耋至蝶褶谍跌踢蹀迭鲽"},
    {"ding", "定丁仃叮啶奠汀灯玎町疔盯碇耵腚葶订酊钉铤锭顶鼎"},
    {"diu", "丢铥"},
    {"dong", "动东侗冬冻咚垌岽峒恫懂栋桐氡洞甬硐筒胨胴董酮鸫"},
    {"dou", "都兜投抖斗痘窦窬篼蔸蚪读豆逗逾钭陡"},
    {"du", "都嘟土堵妒宅度杜椟橐毒渎渡牍犊独督睹碡竺笃纛肚芏蠹读赌镀顿髑黩"},
    {"duan", "断椴段煅短端簖缎踹锻"},
    {"dui", "对兑堆怼憝敦槌碓追镦队"},
    {"dun", "俊吨囤墩敦沌炖盹盾砘礅豚趸蹲遁钝镦顿"},
    {"duo", "多兑剁咄哆哚垛堕夺度惰捶掇揣朵杂柁棰沱缍舵裰跺踱躲酡铎陀隋驮"},
    {"e", "侉俄厄叱呃哦啊啐噩垩娥婀屙峨庵恶愕扼曷歹猗疴硪胺腭苊莪萼蛤蛾讹谔轭遏"
          "邑鄂锇锷阏阿隘颚额饿鬲鳄鹅鹗"},
    {"ei", "诶"},
    {"en", "恩摁蒽"},
    {"er", "而儿尔二佴洱濡珥耳贰迩铒饵鲕鸸"},
    {"fa", "发法乏伐垡拔泛珐砝筏罚阀"},
    {"fan", "凡反帆幡拚梵樊泛潘烦燔犯畈番矾繁翻范蕃藩蘩蟠袢贩蹯返钒饭"},
    {"fang", "方仿匚坊妨彷房放枋纺肪舫芳访邡钫防鲂"},
    {"fei",
     "匪吠啡妃废怫悱扉拂斐榧沸淝狒痱砩祓篚绯翡肥肺腓芾茇菲蜚裴诽费镄霏非飞鲱"},
    {"fen", "分份偾匪吩坟奋奔忿愍愤扮拚棼氛汾瀵焚燔玢盼粉粪纷芬酚鲼鼢"},
    {"feng", "方丰俸冯凤唪奉封峰捧枫沣泛烽疯砜缝葑蚌蜂讽逄逢酆锋风"},
    {"fo", "佛"},
    {"fou", "不否缶"},
    {"fu", "不还夫仅付伏佛俘俯傅凫副包匐呋咐哺复妇孚孵宓富市幅幞府弗彳怀怫扶抚"
           "拂拊掊敷斧服桴氟沸浮涪溥滏父甫砩祓福稃符绂绋缚罘肤脯腐腑腹艴芙芾苻"
           "茯莆莩菔蚨蜉蝠蝮袱覆讣负赋赙赴趺跗辅辐郛釜阜阝附鞴馥驸鲋鳆麸黻黼"},
    {"ga", "伽呷咖嘎噶夹尕尜尬戛旮胳轧钆"},
    {"gai", "丐咳垓戤改核概汽溉盖胲芥该赅钙陔骸"},
    {"gan", "个感乾坩奸尴干捍擀敢旰杆柑橄汗泔淦澉甘疳矸秆竿绀肝苷赣赶酐"},
    {"gang", "亢伉冈刚岗戆扛抗杠港筻纲缸罡肛肮钢"},
    {"gao", "高告咎搞杲槁槔浩皋睾稿篙糕缟羔膏蒿藁诰郜锆镐"},
    {"ge", "个可介仡假割各合咯哥哿嗝噶圪塥屹戈搁搿格歌浩疙盖砝硌纥胳膈舸菏葛虼"
           "蛤袼铬镉阁隔革颌骼髂鬲鸽"},
    {"gei", "给"},
    {"gen", "亘哏根痕艮茛跟"},
    {"geng", "亘亢哽埂庚更梗硬绠羹耕耿赓邢颈鲠"},
    {"gong", "公工红供共功咣宫巩廾弓恭拱攻杠汞珙磺肱虹蚣蛩觥贡躬龚"},
    {"gou", "佝勾句垢够媾岣彀拘构枸沟狗笱篝缑苟觏诟购遘钩鞲"},
    {"gu", "家估古告呱咕哌嘏固姑孤崮故枯梏毂汩沽滑牯牿瓠痼皋瞽箍罟股胍臌苦菇菰"
           "蛄蛊角觚诂谷贾轱辜酤钴锢雇顾骨骰鲴鸪鹄鹘鼓"},
    {"gua", "刮剐卦呱寡惴括挂栝瓜聒胍舌褂诖鸹"},
    {"guai", "乖怪拐掴"},
    {"guan", "果串倌关冠官惯掼斡棺涫灌盥矜管纶罐莞菅观贯馆鳏鹳"},
    {"guang", "光咣广恍桄横潢犷胱逛"},
    {"gui", "傀刽刿匦匮哇圭妫娃宄庋归撅晷柜桂桅桧概洼炅炔瑰癸皈硅祈簋规觖诡贵跪"
            "蹶轨闺隗鬼鲑鳜龟"},
    {"gun", "丨卷棍混滚磙绲衮辊鲧"},
    {"guo", "国过活果划呙唬囗埚崞帼掴椁涡猓聒虢蜮蜾蝈蠃裹郭锅馘"},
    {"ha", "吓呵哈獬虾蛤铪"},
    {"hai", "还孩亥咳咴嗨害氦海胲醢骇骸"},
    {"han", "感函厂含喊寒嵌悍憨憾捍撖撼旰旱晗汉汗泔涵淦澉瀚焊焓犴甘矸罕翰菡蚶邗"
            "邯酣阚韩顸颔鼾"},
    {"hang", "行吭夯巷杭桁沆炕狠狼珩绗肮航酐颃"},
    {"hao", "好号唬嗥嚆嚎壕妞昊毫浩濠灏皋皓睾耗蒿薅蚝豪貉郝镐颢"},
    {"he", "和何劾合吓呵呼哈哧喝嗑嗬壑害揭曷核格河洽涸渴盍盒硅禾纥翮苛荷菏藿蚵"
           "蝎褐诃貉贺赫阂阖霍颌鹤"},
    {"hei", "嗨嘿黑"},
    {"hen", "很哏恨掀狠痕艮"},
    {"heng", "行亨哼恒桁横珩蘅衡訇"},
    {"hng", "哼"},
    {"hong", "红共哄宏弘汪泓洚洪港烘荭蕻薨虹訇讧轰闳鸿黉"},
    {"hou", "后侯候厚吼喉堠後猴瘊篌糇逅骺鲎"},
    {"hu", "和乎互冱呼唬唿囫壶岵弧忽怙惚戏户戽扈护斛核槲汩沪浒湖滹烀煳狐猢琥瑚"
           "瓠祜笏糊羽胍胡芦芴苦葫虍虎蝴觳许轷醐雇鹄鹕鹘鹱"},
    {"hua", "话侉划劐化华叱哇哗找敌桦滑猾画砉稞花豁铧骅"},
    {"huai", "划喟圳坏坯徊怀槐淮踝"},
    {"huan", "还唤圜垸奂宦寰幻患换援擐桓欢洹浣涣漶灌焕獾环瑗痪皖眩缓缳脘萑豢逭"
             "郇锾鬟鲩"},
    {"huang", "凰幌徨恍惶慌晃横湟潢煌璜癀皇磺篁簧肓芒茫荒蝗蟥谎遑隍鳇黄"},
    {"hui", "会回卉咴哕喙堕彗徊徽恚恢悔惠慧挥晖晦桧毁汇洄浍涣溃灰烩珲皓眭睢秽绘"
            "缋茴荟蕙虫虺蛔蟪讳诙诲贿辉隳麾"},
    {"hun", "婚捆昆昏棍浑混溷珲荤诨阍馄魂"},
    {"huo", "和活伙劐化呵嚯壑夥惑或扮攉火灬瓠砉祸耠获藿蠖豁货越钬锪镬霍"},
    {"ji", "己期给其几机及丌乩亟伎佶倚偈冀击剂剞卟即厝叽吉咭哜唧圾基墼奇妓姬嫉"
           "季寂寄居屐岌嵇嵴彐忌急悸戟戢技挤掎揖既暨极棋棘楫殛汲洁洎济激犄猗玑"
           "畸畿疵疾瘠瘵睽瞿矶祭积秸稷稽笄笈箕籍粢系级纪继绩缉羁肌脊脔芨芰荠萁"
           "蒺蓟蕺藉虮蜡觊计讥记诘赍跻跽辑迹郅际隔集霁革饥骥髻鲚鲫鸡麂齐齑"},
    {"jia", "家价伽佳假加呷咖嘉嘏夏夹嫁岬恝戛押拮挈挟揩暇架枷柙浃珈甲痂瘕稼笳胛"
            "茄荚葭蛱袈袷贾跏迦郏钾铗镓颊驾骱"},
    {"jian", "前间见件俭健僭兼减剑剪咸喊囝坚奸孱尖建戋戬拣捡搛枧柬检楗槛歼毽沮"
             "浅涧渐湔溅煎牮犍犴监睑硷碱笕笺简箭箴缄缣翦肩腱舰艰茛茧荐菅蒹裥謇"
             "谏谫贱趼践踺蹇鉴锏键鞯饯鲣鹣"},
    {"jiang", "将僵匠奖姜强桨江洚浆犟疆礓糨绛缰耩茳蒋虹讲豇酱降"},
    {"jiao", "交佼侥僬剿叫叽咬噍嚼妖姣娇峤徼挢搅教敫校椒浇湫激焦爝狡皎矫礁窖绞"
             "缴胶脚艽茭菽蕉蛟觉角跤轿较郊酵醮铰饺骄鲛鹪"},
    {"jie",
     "她家接解亥介价借假偈偕劫卩唧喈嗟圾契姐婕孑届差戒截担拮拾捷揭暨杰桀桔楷概"
     "洁渴獬界疖疥皆睫砝碣祖秸竭籍结羯节芥苴藉蚧街袷讦诘诫阶颉骱髻鲒"},
    {"jin", "进仅今劲卺吟噤堇妗尽巾廑斤晋槿津浸湛烬瑾矜禁筋紧缙肋荩衿襟觐谨赆近"
            "金钅锦靳馑"},
    {"jing", "经井京儆兢净刭劲境婧弪径惊憬敬旌晟景晶檠氏泾獍痉睛竞竟箐粳精肼胫"
             "腈茎荆菁蜻警迳醒镜阱青靓靖静颈鲸"},
    {"jiong", "冂坷垧扃炅炯窘迥"},
    {"jiu", "就久九僦厩咎啾噍愁揪救旧柩桕湫灸玖疚究纠臼舅蝤赳蹴酒阄韭鬏鸠鹫"},
    {"ju",
     "车且举仇佝俱倨具剧句告咀姐娶局居屈屦巨惧拒拘拱据掬枸柜桔椐榉榘橘沮渠炬犋"
     "狙琚疽瞿矩租窭聚苣苴莒菊菹蘧蛆裾讵趄足距踞踽遽鄹醵钜锔锯雎鞠鞫飓驹鬻龃"},
    {"juan", "身倦卷圈娟捐擐桊泫涓狷甄眩眷睃绢蕊蜷蠲鄄锩镌隽鹃"},
    {"jue", "乙倔决劂厥嗟噘噱嚼孓屈崛抉掘撅攫柽桷梏橛爝爵狂獗珏矍穴绝脚蕞蕨蛙蠼"
            "觉角觖觳诀谲蹶镢"},
    {"jun", "俊军匀卷君均峻捃旬浚狻皲睃竣筠菌訇郡钧隽骏麇龟"},
    {"ka", "佧卡咔咖咯喀胩"},
    {"kai", "开凯剀劾喝垲岂忾恺慨揩核楷渴溘蒈铠锎锴雉"},
    {"kan", "看侃凵刊勘喊坎堪嵌戡槛瞰砍莰阚龛"},
    {"kang", "亢伉坑奋康慷扛抗杭沆炕糠荒钪闶"},
    {"kao", "尻拷搞栲槁烤犒考铐靠"},
    {"ke", "可克刻呵咳喀嗑坷壳客岢恪柯棵氪渴溘珂疴盍瞌碣磕科稞窠缂苛蚵蝌课轲钶"
           "锞颏颗骒髁"},
    {"kei", "刻"},
    {"ken", "啃垠垦恳狠肯裉龈"},
    {"keng", "吭坑忐硎铿"},
    {"kong", "倥孔崆恐控穹空箜腔"},
    {"kou", "口佝刳叩寇彀扣抠挎眍筘芤蔻"},
    {"ku", "刳古哭喾圣堀库挎掘枯窟绔苦裤跨酷骷"},
    {"kua", "侉垮夸挎胯跨髁"},
    {"kuai", "会侩傀哙块快浍狯筷脍蒯郐魁"},
    {"kuan", "完宽棵款髋"},
    {"kuang", "兄况匡呈哐圹夼旷枉框湟狂眶矿磺筐纩诓诳贶逛邝"},
    {"kui", "亏傀匮喟喹夔奎岿悝愦愧揆暌溃盔睽窥篑缺聩臾葵蒉蝰觖跬踩逵隗馈馗魁"},
    {"kun", "卵困坤悃捆昆混琨醌锟阃髡鲲"},
    {"kuo", "廓扩括栝蛞适阔"},
    {"la", "剌啦喇垃拉摺旯瘌砬腊落蓝蜡辣邋"},
    {"lai", "来崃徕涞濑癞睐籁莱赉赖铼黧"},
    {"lan", "兰啉婪岚懒懔拦揽斓栏榄滥漤澜烂篮缆罱蓝褴览谰郴镧阑"},
    {"lang", "啷廊朗榔浪狼琅稂羹莨蒗螂踉郎锒阆"},
    {"lao", "老佬僚劳唠姥崂捞撩栳涝潦烙牢獠痨络耢落蓼酪醪铑铹"},
    {"le", "了乐仂勒叻嘞泐肋鳓"},
    {"lei", "儡勒嘞垒嫘擂檑泪漯磊类累缧羸耒肋蕾诔酹镭雷"},
    {"len", "啉"},
    {"leng", "冷塄愣棱楞"},
    {"li", "里位力理丽仂例俐俚俪傈列利励历厉厘叻吏呖唳喱坜娌嫠悝戾捩李枥栎栗梨"
           "沥泣溧漓澧犁狸猁珞璃疠疬痢砬砺砾硌礼离立笠篥篱粒粝缡罹翮苈荔莅莉蓠"
           "藜蛎蜊蠡詈跞轹逦郦醴锂隶雳霾骊鬲鲡鲤鳢鹂黎黧"},
    {"lia", "俩"},
    {"lian",
     "令奁帘廉怜恋搛敛楝殓涟潋濂炼琏瞵练羸联脸膦臁苓莲蔹蠊裢裣连链镰零鲢"},
    {"liang", "两亮俩凉墚惊晾梁椋粮粱良莨谅踉辆量靓魉"},
    {"liao", "了佬僚嘹寥寮尥廖撂撩料潦燎獠疗缭聊蓼辽钌镣鹩"},
    {"lie", "例冽列劣咧埒捩栗洌烈猎累膊裂趔躐邋鬣"},
    {"lin", "林临任凛吝啉嶙廪懔拎檩淋琳瞵磷粼膦蔺赁躏辚遴邻霖鳞麟"},
    {"ling",
     "令伶冷凌另呤囹岭怜拎柃棂棱泠灵玲瓴磷绫羚翎聆苓菱蛉酃铃陵零领鲮龄"},
    {"liu", "六刘旒柳榴泖泵流浏游溜熘琉留瘤硐硫碌绺聊蓼遛鎏锍镏陆馏骝鹨"},
    {"lo", "咯"},
    {"long", "咙垄垅弄拢栊泷珑癃砻窿笼聋胧茏陇隆龙"},
    {"lou", "偻喽娄嵝搂楼漏牢瘘篓耧蒌蝼镂陋露髅"},
    {"lu", "六路卢卤噜垆庐录戮掳撸栌橹氇泸渌漉潞炉璐瘳碌禄簏绿胪舻芦蓼虏角谷赂"
           "轳辂辘逯酪镥陆露颅鲁鲈鸬鹭鹿麓"},
    {"luan", "乱卵娈孪峦挛栾滦脔銮鸾"},
    {"lun", "仑伦囵抡沦纶论轮"},
    {"luo", "果路倮咯捋摞格椤橐泺洛漯烙猓猡珞瘰硌碌箩络罗脶荦萝落蜾螺蠃蠡袼裸跞"
            "逻酪锣镙雒骆骡"},
    {"lv", "侣偻吕屡履律捋旅榈氯滤率稆累绿缕膂虑褛铝闾驴鹿"},
    {"lve", "掠率略锊"},
    {"m", "呒唔"},
    {"ma", "么吗唛嘛妈嬷抹摩杩犸玛码蚂蟆貉貊靡马骂麻麽"},
    {"mai", "派买劢卖咪唛埋脉荬迈霾麦"},
    {"man", "埋墁幔幕慢曼满漫熳瞒缦蔓蛮螨谩镘鞔颟馒鳗"},
    {"mang", "忙朦氓漭盲瞢硭芒茫莽蟒邙"},
    {"mao", "侔冒勖卯峁帽懋描旄昴毛泖牟牦猫瑁瞀矛耄耗茂茅茆蛑蝥蟊袤貌贸铆锚髦"},
    {"me", "没么末麽"},
    {"mei", "没美每味坶墨妹媒媚寐嵋昧枚某梅楣浼湄煤猸玫眉糜莓袂谜酶镁镅霉魅鹛"},
    {"men", "们门懑扪汶焖钔闷鞔"},
    {"meng", "明勐孟懵朦梦檬氓猛甍盟瞑瞢礞艋艨萌蒙虻蜢蟊蟒蠓锰黾"},
    {"mi",
     "冖咪嘧宓密幂幺弥弭摩敉汨泌溟猕眯祢秘米糜糸縻脒芈蘼蜜觅谜谧辟迷醚靡麋"},
    {"mian", "面免冕冥勉娩宀棉沔泯渑湎眄眠瞑绵缅腼黾"},
    {"miao", "吵喵妙庙描杪淼渺猫眇瞄秒缈缪苗藐蜱邈鹋"},
    {"mie", "乜咩咪灭篾蔑蠛"},
    {"min", "岷悯愍抿敏民汶泯玟珉皿眠缗苠闵闽鳘黾"},
    {"ming", "明名冥命暝溟皿盟瞑茗萌螟酩铭鸣"},
    {"miu", "缪谬"},
    {"mo", "没无万么伯佰冒勿嘿墨嫫嬷寞帕抹摩摸摹昧末模殁沫漠瘼百磨秣耱脉膜茉莫"
           "蓦藐蘑蟆袜谟貉貊貌貘镆陌馍魔麽默"},
    {"mou", "件侔厶哞婺某毋牟眸缪蛑袤谋鍪"},
    {"mu", "目亩仫募嘿坶墓姆姥婺幕慕拇暮木模母毪沐牟牡牧睦穆苜莫钼"},
    {"n", "哏哽唔嗯"},
    {"na", "那内南呐呶哪娜拿捺箬絮纳肭衲钠镎"},
    {"nai", "那能乃佴哪奈奶柰氖耐艿萘鼐"},
    {"nan", "冉南喃囝囡楠男罱腩蝻赧难"},
    {"nang", "囊囔攮曩馕"},
    {"nao", "呶垴孬恼挠淖猱瑙硇脑蛲铙闹"},
    {"ne", "那呐呢哪疒疔讷"},
    {"nei", "那内哪馁"},
    {"nen", "嫩恁枘"},
    {"neng", "而能耐"},
    {"ng", "哽唔嗯"},
    {"ni", "你伲倪匿呢坭妮尼嶷怩慝拟旎昵泥溺猊睨祢腻逆铌霓鲵"},
    {"nian", "年埝廿念拈捻撵碾粘蔫趁辇辗鲇鲶黏"},
    {"niang", "娘酿"},
    {"niao", "嬲尥尿溺脲茑袅鸟"},
    {"nie", "乜倪哪啮嗫囡埝孽幸捏捻泥涅聂臬蘖蹑镊镍陧颞"},
    {"nin", "恁您"},
    {"ning", "年佞冰凝咛宁拧攘柠泞泥狞甯疑聍"},
    {"niu", "妞忸扭拗牛狃纽蚴钮"},
    {"nong", "侬农咔哝弄浓脓"},
    {"nou", "耨"},
    {"nu", "仅努呶奴孥帑弩怒肭胬褥驽"},
    {"nuan", "暖暧濡"},
    {"nuo", "那傩呐哪喏娜懦挪掉搦濡糯诺锘需"},
    {"nv", "女恧狃絮胬衄钕"},
    {"nve", "疟虐"},
    {"o", "哦喔噢"},
    {"ou", "偶区呕怄握欧殴沤渥瓯耦藕讴遇鸥"},
    {"pa", "把派叭吧啪帕怕扒杷爬琶筢耙芭葩趴钯"},
    {"pai", "派俳哌啡徘拍排湃牌脾蒎迫"},
    {"pan", "伴判半卞叛弁彦扳拌拚攀泮潘爿片畔番皤盘盼磐繁胖般蟠袢襻蹒鄱"},
    {"pang", "方乓仿傍庞彭彷房旁榜滂磅耪胖膀蒡螃逄逢"},
    {"pao", "刨包匏咆庖抛抱泡炮狍疱胞脬苞袍趵跑"},
    {"pei", "佩倍呸啡坏培妃帔掊旆沛淠肺胚艴茇蜚裴赔辔配醅锫陪霈"},
    {"pen", "吩喷汾湓盆"},
    {"peng", "亨傍嘭堋庄彭怦抨捧旁朋棚榜滂澎烹砰硼碰篷膨苹蓬蟛逢鹏"},
    {"pi",
     "被比丕仳俾僻副劈匹卑吡否啤噼圮坏坯埤培媲屁帔庀庇庳扑批披拂擗枇毗淠濞琵甓"
     "番疋疲痞痦癖皮睥砒篦纰罴脾芘苤萆蕃薜蚌蚍蜱裨譬貔辟邳郫鄱铍陂陴霹鼙"},
    {"pian", "便平偏扁片犏篇缏翩胼蝙褊谝蹁辨骈骗"},
    {"piao", "剽嘌嫖朴殍漂瓢瞟票缥膘莩螵飘骠髟"},
    {"pie", "丿撇氕瞥苤蔽"},
    {"pin", "匕品姘娉嫔拚拼榀泵牝聘贫频颦"},
    {"ping", "平乒俜冯凭坪堋娉屏枰瓶砰秤聘苹萍评鲆"},
    {"po", "剖叵坡婆朴泊泺泼溥珀番皤破笸粕繁膊跛迫鄱钋钷陂霸颇魄"},
    {"pou", "部剖培抱掊涪瓿裒踣"},
    {"pu",
     "仆剥匍卜噗圃埔堡扑扶攴攵普暴曝朴氆浦溥濮瀑璞甫脯苻莆菩葡蒲谱蹼铺镤镨"},
    {"qi",
     "起己期其气七丌乞亓亟企伎俟偈凄切刺勤吃启吱嘁器圻奇契妻宿屺岂岐崎弃忮忾恝"
     "憩戚技抵挈揭支旗杞枝柒栖桤棋槭欹欺歧汔汽沏泣淇溪漆琦琪甭畦畸砌碛示祁祈祺"
     "稽綦綮绮缉耆脐芑芪荠萁萋葺蕲蛴蜞讫趿蹊迄逗颀骐骑鳍麒齐"},
    {"qia", "卡咭客恰挈掐洽疴葜袷髂"},
    {"qian", "前乾仟佥倩凵千堑寨岍嵌忏悭愆慊扦掮搴撖柑椠欠歉浅涔湔潜牵犍筋签箝"
             "纤缱肷腱芊芡茜荨虔褰谦谴赶迁遣钎钤钱钳铅阡骞黔"},
    {"qiang", "将丬呛哐墙嫱强戕戗抢控枪樯炝爿箐羌羟腔蔷蜣襁跄跫锖锵镪"},
    {"qiao", "乔侨俏削劁壳峤峭巧悄愀愁憔招捎搞撬敫敲校桥樵橇毳焦瞧硗硝窍缲翘茭"
             "荞蕉诮谯跤跷醮锹雀鞒鞘"},
    {"qie", "且伽切唼喋契妾婕怯惬慊挈捷沏渫漆砌窃箧脞茄蕺趄郄锲"},
    {"qin", "亲侵勤吣嗪噙堇寝廑揿擒槿檎沁浸溱琴矜禽秦芩芹蓁螓衾衿覃钦锓"},
    {"qing", "情亲声倩倾卿圊庆擎晴檠氢氰清磬箐精綮罄胜苘蜻謦请轻青顷鲭黥"},
    {"qiong", "琼穷穹筇芎茕蛩跫邛銎鞠"},
    {"qiu", "丘仇俅囚团巯惆愀楸氽求泅湫犰球秋糗艽虬蚯蝤裘赇逑遒邱邺酋馗鳅鼽龟"},
    {"qu", "去劬区取句娶屈岖巨戌曲朐枸氍渠璩癯瞿磲祛苣蕖蘧蛆蛐蜡蠼衢觑诎趋趣躯"
           "遽阒鞠鞫驱鸲麴黢龋"},
    {"quan", "全串券劝卷圈圳悛拳拴权栓桊泉犬犭獾畎痊筌绻荃蜷诠辁醛铨颧鬈"},
    {"que", "却屈悫攉榷炔猎瘸确缺舄芍觳阕阙雀鹊"},
    {"qun", "群裙蹲逡遁麇"},
    {"ran", "然冉染燃苒蚺髯"},
    {"rang", "嚷壤攘瓤禳穰让"},
    {"rao", "娆扰桡绕荛饶"},
    {"re", "偌喏惹热若"},
    {"ren", "人儿亻仁仞任刃壬妊忍恁稔纫荏葚衽认轫韧饪"},
    {"reng", "仍戎扔穰耳艿"},
    {"ri", "日"},
    {"rong", "冗容嵘戎榕溶熔狨绒肜茸荣蓉蝾融隔"},
    {"rou", "揉柔糅肉蹂鞣"},
    {"ru", "如女月乳儒入嚅孺汝洳溽濡缛肉茹蓐薷蠕褥襦辱铷需颥"},
    {"ruan", "朊濡软阮需"},
    {"rui", "兑内枘瑞睿芮蕊蕤蚋锐"},
    {"run", "润闰"},
    {"ruo", "偌弱惹溺箬芮若"},
    {"sa", "仨卅挲撒檫洒脎萨蔡趿飒"},
    {"sai", "噻塞思腮赛鳃"},
    {"san", "三伞叁散毵糁霰馓"},
    {"sang", "丧嗓搡桑磉颡"},
    {"sao", "哨埽嫂扫搔梢燥瘙缫缲臊骚鳋"},
    {"se", "啬塞寨槭泣涩瑟穑色铯"},
    {"sen", "森洒"},
    {"seng", "僧"},
    {"sha", "接傻刹厦哈唼啥嗄噎挲杀杉歃沙煞痧砂纱莎裟铩霎鲨"},
    {"shai", "晒筛色酾"},
    {"shan", "儋删剡单善埏姗嬗山彡扇掸掺擅杉栅檀汕潸澹煽珊疝禅缮膳膻舢芟苫蟮衫"
             "讪赡跚邓鄯钐闪陕骟髟鳝"},
    {"shang", "上伤商垧墒尚晌殇汤熵绱裳觞赏"},
    {"shao", "削劭勺召哨少招捎搜杓梢溲潲烧稍笤筲绍艄芍苕蛸裢邵鞘韶"},
    {"she", "佘厍奢射慑折拾揲摄歙涉滠猞畲睫碟社舌舍蛇蛞设赊赦邪麝"},
    {"shei", "谁"},
    {"shen",
     "什身信伸参吲呻哂娠婶审慎抻椹沈深渖渗湛甚申矧砷神糁绅肾胂莘葚蜃诜谂震"},
    {"sheng", "生声丞乘冼剩升圣垩姓媵嵊晟渑牲甥甸盛省眚笙绳胜"},
    {"shi", "是时事斯世什使十实仕似侍势匙史唑嗜嘘噬埘堤士失始室寺尸屎峙市师式弑"
            "彖恃拭拾挈提施柿殖氏汁液湿炻狮矢石示礻筮耆肢舍舐莳蓍虱蚀螫视誓识试"
            "诗谥豉豕贳赫轼适逝郝酾释铈食饣饰驶鲥鲺"},
    {"shou", "手兽受售守寿扌授收熟狩瘦绶艏首"},
    {"shu", "书俞倏叔售嗽塾墅姝娶孰属庶恕戍抒揄摅数暑曙术朱束杼枢树梳殊殳毹沭涑"
            "淑漱澍熟疋疏秫竖纾署腧舒荼菽蔬薯蜀豫赎输述透野除黍鼠"},
    {"shua", "刷唆唰涮耍"},
    {"shuai", "帅摔率甩蟀衰"},
    {"shuan", "拴栓汕涮踹闩"},
    {"shuang", "双孀泷淙爽霜"},
    {"shui", "说水氵睡税谁"},
    {"shun", "俊吮巛巡恂盾瞬舜顺"},
    {"shuo", "说勺嗍嗽妁搠数朔杓槊溯濯烁硕蒴铄"},
    {"si", "以斯四已丝伺似俟兕厕厮厶台司咝嗣嘶姒寺巳徙思撕析死汜泗澌祀祠私笥糸"
           "纟缌耜肄肆菥蛳锶雉食饲驷鸶"},
    {"song", "凇宋崧嵩忪怂悚松淞竦耸菘讼诵送颂"},
    {"sou", "叟嗖嗽嗾搜擞敕族涑溲瞍艘薮螋锼飕馊"},
    {"su", "俗僳嗉嗖塑夙宿愫搬涑溯稣簌粟素缩肃苏蓿蔌觫诉谡速酥"},
    {"suan", "撰狻算蒜酸"},
    {"sui", "尿岁彗濉燧眭睢碎祟穗粹绥荽莎蓑虽谇遂邃隋随隧髓"},
    {"sun", "孙损榫狲笋荪跣隼飧餐"},
    {"suo", "些所唆唢嗍嗦娑抄挲桫梭沙琐睃索缩羧莎蓑衰逡锁霍"},
    {"ta", "他她它太哈嗒塌塔拓挞搭榻沓溻漯獭趿踏蹋达遢铊闼鳎"},
    {"tai", "大能太台呔态抬汰泰炱肽胎苔薹跆邰酞钛骀鲐"},
    {"tan",
     "但叹坍坛坦弹忐探摊昙檀毯沈淡湛滩潭澹炎炭痰瘫碳胆舔蕈袒覃谈谭贪郯钽锬镡"},
    {"tang", "倘傥唐堂塘帑惝搪棠樘汤淌溏烫瑭糖羰耥膛螗螳趟躺醣铴镗饧"},
    {"tao", "叨啕套姚挑掏桃洮涛淘滔焘绦萄讨跳逃陶韬饕鼗"},
    {"te", "特匿式忑忒慝铽"},
    {"tei", "忒"},
    {"teng", "滕疼腾藤誊"},
    {"ti",
     "是体倜剃剔啼嚏堤屉弟悌惕折提替梯棣涕狄睇绨缇肆荑裼踢蹄达逖醍锑题鹈"},
    {"tian", "天佃典吞嗔填忝恬掭栝殄沾添滇甜田甸町畋腆舔苫蚕蚺钿阗"},
    {"tiao", "佻啁姚挑条桃眺祧稠窕笤粜苕蜩调超跳踔迢髫鲦龆"},
    {"tie", "占帖怙萜蝶贴铁餮"},
    {"ting", "亭停厅听奠婷庭廷挺梃汀烃町艇莛葶蜓铤霆"},
    {"tong", "重同仝佟侗僮嗵垌峒彤恫恸恿捅桐桶洞潼痛瞳砼硐童筒统艟茼通酮铜"},
    {"tou", "头亠偷愉投透逗钭骰"},
    {"tu", "余兔凸吐图土堍屠徒杜涂秃突荼菟跌途酴钍"},
    {"tuan", "团彖抟揣敦湍疃痪税"},
    {"tui", "弟忒推煺税脱腿蜕褪追退颓"},
    {"tun", "吞吨吴囤屯敦暾氽沌炖窀肫臀褪豚逐饨"},
    {"tuo", "他它乇佗唾坨妥庹惰托拓拖柁柝椭橐池沱沲砣税箨脱舄蛇跎迤酡铊陀隋驮驼"
            "魄鸵鼍"},
    {"wa", "佤凹哇娃娲挖洼瓦腽蛙袜鞋"},
    {"wai", "外夭崴歪"},
    {"wan",
     "万丸免剜园娩婉完宛弯惋挽晚朊湾烷玩琬畹皖碗箢纨绾脘腕芄莞菀蔓蜿豌顽"},
    {"wang", "方亡匡妄尢往忘惘旺望枉汪王皇网罔芒辋魍"},
    {"wei", "有为于位机伟伪倭偎卫危味唯喂囗围圩堤委威娓尉尾崴嵬巍帏帷微惟慰未桅"
            "沩洧涠渭潍炜煨熨猗猥猬玮畏痿眭睢立纬维胃艉芟苇荽萎葳蔚薇诿谓軎违逶"
            "遗闱阢隈隗隹韦韪魏鲔"},
    {"wen", "问文眼免刎吻娩愠昧殁汶温玟璺瘟稳笏紊纹蚊闻阌限雯"},
    {"weng", "嗡壅瓮翁蓊蕹"},
    {"wo", "我倭卧喔嗌夭媪幄挝握斡杌沃涡渥瘟硪窝肟莴蜗龌"},
    {"wu",
     "无物乌五亡仡仵伍侉侮兀务勿午吴吾呒呜唔喔圬坞妩婺寤屋巫庑忤怃恶悟戊捂於旄"
     "晤杌梧武毋母污浯渥焐牾痦瞀笏舞芜芴蜈蝥诬误迕邬鋈钨阢雾骛鹉鹜鼯"},
    {"xi", "西习僖兮卤吸咦咭唏喜嘻夕奚媳嬉屎屣嵇希席徙息悉惜戏撕既昔晰曦析栖樨"
           "檄欷歙汐洒洗浠淅溪烯熄熙熹牺犀猎玺皙矽硒禊禧稀穸粞系细羲翕腊膝舄舾"
           "茜菥葸蓰蜥蜴螅蟋袭裼褶觋蹊郄郗醯铣锡阋隙隰饩鼷"},
    {"xia",
     "下侠假匣厦吓呀呷呼哧唬嗄嗑夏岈峡押斜暇柙歃毳狎狭瑕瘕瞎硖罅葭虾辖遐霞黠"},
    {"xian", "现见先仙冼县咸妗娴嫌宪寰岘弦彡慊捍掀探显暹氙洒洗涎濂燹猃献痫省矣"
             "碱祆筅籼纤线羡肩腺舷苋莶藓蚬衔贤跣跹酰铣锨锬闲限险陷霰馅鲜鹇黹"},
    {"xiang",
     "想相向像乡亨享厢响巷庠攘橡洋湘祥箱缃翔舡芗葙蟓襄详象镶降项飨饷香骧鲞"},
    {"xiao", "小佼俏削叟号呼咻哓哨哮唬啸嚣姣孝宵崤捎搜效晓枭枵校梢消淆潇爻狡硝"
             "笑筱箫绡肖胶芍茭萧蛸逍销霄骁魈"},
    {"xie", "些接解亵偕写勰协卸叶唏喈契屑廨懈挟携摺撷斜桔械楔榍榭歇歙汁泄泻渫溉"
            "瀣燮獬眭绁缬耶胁苴薤蝎蟹血谐谢豫跬躞迦邂邪隰鞋颉骱鲑"},
    {"xin", "心信新囟寻忄忻昕欣款歆芯莘薪衅辛鑫锌镡馨"},
    {"xing", "行兴刑型姓幸形性悻惺擤星杏猩省研硎胜腥荇荥邢醒陉饧"},
    {"xiong", "能兄凶匈宪汹熊胸芎雄"},
    {"xiu", "休修咻嗅宿岫庥朽溴煦秀绣羞臭莠袖貅锈馐髹鸺"},
    {"xu", "于休余勖叙吁呼咻嘘圩墟姐婿序徐怵恤戌旭旮朐栩洫浒溆煦畜盱砉糈絮绪续"
           "肷胥芋蓄蓿虚许诩邪酗醑雩需须顼馘"},
    {"xuan",
     "亘儇券喧宣悬揎撰擐旋昕暄暖楦泫洵涓渲滋漩炫煊玄璇痃癣眩碹绚萱谖轩选铉镟"},
    {"xue", "学削哮噱嚯泶炔穴薛血谑踅雪靴鳕"},
    {"xun", "勋埙寻峋巡巽徇循恂悛旬曛梭殉汛洒洵浔浚潭熏狻獯窨荀荤荨蕈薰训讯询迅"
            "逊逡遁郇醺鑫驯鲟"},
    {"ya", "丫亚伢压吖吾呀哑垭娅岈崖御押揠札桠歇氩涯牙琊疋痖睚砑碣芽蚜衙讶轧迓"
           "邪雅鸦鸭"},
    {"yan", "但眼严俨俺偃兖剡厂厌厣咽唁埏埯堰奄妍嫣宴岩崦巡广庵延彦恹掩揞晏檐殷"
            "氤沿洇涎淡淫淹湮滟演炎烟焉焰焱燕狠琰癌盐研砚筵罨羡胭腌艳芫菸蔫蜒衍"
            "覃言讠谚谳赝趼郾鄢酽铅闫阉阎阏阽雁颜餍验魇鼹"},
    {"yang", "样仰佯养央徉怏恙扬昂映杨殃氧泱洋漾炀烊疡痒秧羊英蛘阳鞅鸯"},
    {"yao", "要么佻侥吆咬夭妖姚尧崤崾幺幼徭徼揄摇曜杳洮淫瀹爻珧瑶由疟窈窑繇约耀"
            "肴腰舀药谣轺遥邀钥铫陶鳐鹞"},
    {"ye",
     "也业冶叶咽喝噎墅夜射拽掖揞揲揶斜晔曳椰洇涂液烨爷耶腋荼谒邪邺野铘靥页"},
    {"yi", "一也以意它已丿义乙亦亿仡仪伊佗佚佾依倚刈劓医印台叹听呓咦咿嗌噎噫圪"
           "圯坨埸壹夕失夷奇奕姨姬宜射尾屹峄崎嶷巳异弈弋彝役忆怠怡怿悒懿抑挹掎"
           "揖搋施旖易椅欹殪毅汽沂泄洫渫溢漪焉焱熙熠犄猗疑疙疫痍瘗癔益眙矣硪移"
           "绎缢羡羿翊翌翳翼肄胰臆舣艺艾苡荑薏蚁蛇蛾蜴衣衤袂裔议译诒诣谊贻轶迤"
           "迭逸遗邑酏钇铱镒镱隶雉颐食饴驿黝黟"},
    {"yin", "因众印吟听吲喑圻垠垦堙壹夤姻寅尹币廴引殷氤沂洇淫湛湮潭烟狺瘾窨胤芩"
            "茚茵荫蚓言鄞铟银阴隐霪音饮龈"},
    {"ying", "哽嘤央婴媵嬴应影撄映景楹樱滢潆瀛瑛璎甸瘿盈硬缨罂膺英茔荥荧莹莺萤"
             "营萦蓥蝇赢迎逞郢颍颖鹦鹰"},
    {"yo", "哟唷育"},
    {"yong", "用佣俑勇咏喁墉壅容庸恿慵拥永泳涌甬痈臃臾蕹蛹踊遇邕镛雍饔鳙"},
    {"you", "有又优佑侑卣友右叹呦囿坳奥宥尢尤幼幽忧悠扰揄攸柚油泅游牖犹猷由疣繇"
            "聱莜莠莸蚰蚴蝣蝤诱邮酉釉铀铕鱿黝鼬"},
    {"yu", "于与予亏伛余俞俣吁吾唷喁喻噢圄圉圩域奥妤妪娱宇宛寓尉屿峪崛嵛庾御愈"
           "愉愚懊或拗揄於昙昱栩梧榆欤欲毓毹汩浴淤渔渝澳煜煨熨燠狱狳玉王瑜瘀瘐"
           "盂禹禺窬窳竽粥纡羽聿肀育腧腴臾舁舆舒芋苑菀菸萸蓣蔚虞蜍蜮蝓衙裕觎誉"
           "语谀谕谷豫迂逾遇郁钰阈隅雨雩预饫馀驭鬻鱼鹆鹬龉"},
    {"yuan", "员允元冤原咽园圆圜垣垸塬媛宛怨愿捐掾援橼沅涓渊源爰猿瑗畹眢穿箢缘"
             "芫苑螈袁辕远阮院鸢鸳鼋"},
    {"yue", "说月乐刖哕囝块妁岳悦曰栎樾瀹粤约蜕蠖越跃钥钺阅龠"},
    {"yun", "员云允匀均媪孕宛尉尹怨恽愠昀晕殒氲温熨狁瘟盾筠纭耘芸苑菀蕴运郓郧酝"
            "陨韫韵"},
    {"za", "匝咂咋咱啐嘁扎拶杂砸籴"},
    {"zai", "在才再仔哉宰崽栽灾甾载"},
    {"zan", "咱拶攒昝暂涔湔瓒簪糌赞趱錾"},
    {"zang", "奘戕脏臧葬藏赃驵"},
    {"zao", "凿唣噪早枣槽澡灶燥皂窖糟草藻蚤躁造遭"},
    {"ze", "仄侧则咋啧帻择措昃柞泽稷笮箦舴责赜迮"},
    {"zei", "贼"},
    {"zen", "僭怎谮"},
    {"zeng", "增憎曾甑综缯罾赠锃"},
    {"zha", "乍册吒咋咤哆哳喋喳怍扎插揸札柞查栅楂榨渣渫炸痄眨砟笮膪苴蚱蜡诈轧铡"
            "闸馇齄"},
    {"zhai", "侧债宅寨度择摘斋柴疵瘵砦祭窄翟膪豸"},
    {"zhan", "占孱展崭战搌斩旃栈毡沾湔湛澶盏瞻站粘绽蘸袒詹谵躔辗醮颤"},
    {"zhang", "长丈仉仗嫜嶂帐幛张彰掌杖樟涨漳獐璋瘴章胀蟑账鄣障"},
    {"zhao", "着佻兆召啁嘲找招昭晁朝桃棹沼淖濯照爪笊罩肇著蚤诏赵钊"},
    {"zhe", "这着者乇哲嘀堵庶折摺斥柘浙磔耷著蔗蛰蜇螫褚褶谪赭辄辙遮锗陬鹧"},
    {"zhei", "这"},
    {"zhen", "真侦唇圳坫填帧慎戡振斟朕枕桢椹榛浈溱滇珍甄畛疹砧祯稹箴缜胗臻蓁诊"
             "贞赈趁轸针镇阵震鸩鼎"},
    {"zheng", "正丁丞争奠峥帧征徵怔承拯挣政敞整狰町症睁瞠筝蒸证诤趟郑钲铮鲭"},
    {"zhi", "之只知指伎侄值制卮吱咫址埃埴夂峙帙帜彘徵志忮恃执抵拓挚掷摭支旨昵智"
            "枝枳栀栉桎植止殖氏氐汁治滞炙痔痣直砥示祁祉祗秩积稚窒絷纸织置耆职肢"
            "胝脂膣至致芝芷蛭蜘觯识豸质贽趾跖踬踯轵轾郅酯陟雉骘鸷黹"},
    {"zhong", "中种重仲众冢夂忠忪潼盅童终肿舂舯董蚣螽衷踵钟锺"},
    {"zhou", "侏倜周咒啁啄喙妯宙州帚扭昼柚注洲皱碡祝籀粥繇纣绉肘育胄舟舳荮诌轴"
             "逐酎骤鬻"},
    {"zhu",
     "之丶主予伫住侏助嘱宁属庶拄斗朝术朱杼柠柱株楮槠橥泞注洙渚潴澍炷烛煮猪珠疰"
     "瘃瞩祝竹竺筑箸翥舳苎茁茱著蚰蛀蛛褚诛诸贮躅逐逗邾铢铸阻除驻麈"},
    {"zhua", "抓挝爪"},
    {"zhuai", "拽转"},
    {"zhuan", "专传啭巽撰沌湍砖篆赚转颛馔"},
    {"zhuang", "僮壮奘妆幢庄憧戆撞桩状艟装"},
    {"zhui", "坠垂惴揣椎槌缀缒致萑赘追锥隧隹骓"},
    {"zhun", "准屯敦淳盹窀肫胗谆"},
    {"zhuo",
     "着倬勺卓啄啜拙捉掇擢斫杓桌棹浊浞涿淖濯灼焯琢禚箸缴肫茁著蕞诼趵踔躅酌镯"},
    {"zi", "子自事次仔兹吱呲咨姊姿字孜孳嵫恣柴梓淄渍滋滓甾疵眦秭笫籽粢紫缁耔茈"
           "觜訾谘赀资趑辎锱髭鲻龇"},
    {"zong", "总从偬宗枞棕粽纵综腙踪鬃"},
    {"zou", "走奏揍族楱诹趣邹鄹陬驺鲰"},
    {"zu", "俎卒啐嘁姐族槭沮淬祖租组苴菹诅足蹴镞阻"},
    {"zuan", "撮攥纂缵赚躜钻"},
    {"zui", "最咀嘴堆摧撮罪羧蕞觜醉"},
    {"zun", "奠尊撙樽蹲遵鳟"},
    {"zuo", "作做乍佐凿唑嘬坐左座怍挫撮昨柞琢砟祚笮胙迮酢醋阼"},
    {NULL, NULL}};
#endif


/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
lv_obj_t * lv_ime_pinyin_create(lv_obj_t * parent)
{
    LV_LOG_INFO("begin");
    lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}


/*=====================
 * Setter functions
 *====================*/

/**
 * Set the keyboard of Pinyin input method.
 * @param obj  pointer to a Pinyin input method object
 * @param dict pointer to a Pinyin input method keyboard
 */
void lv_ime_pinyin_set_keyboard(lv_obj_t * obj, lv_obj_t * kb)
{
    if(kb) {
        LV_ASSERT_OBJ(kb, &lv_keyboard_class);
    }

    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    pinyin_ime->kb = kb;
    lv_obj_add_event_cb(pinyin_ime->kb, lv_ime_pinyin_kb_event, LV_EVENT_VALUE_CHANGED, obj);
    lv_obj_align_to(pinyin_ime->cand_panel, pinyin_ime->kb, LV_ALIGN_OUT_TOP_MID, 0, 0);
}

/**
 * Set the dictionary of Pinyin input method.
 * @param obj  pointer to a Pinyin input method object
 * @param dict pointer to a Pinyin input method dictionary
 */
void lv_ime_pinyin_set_dict(lv_obj_t * obj, lv_pinyin_dict_t * dict)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    init_pinyin_dict(obj, dict);
}

/**
 * Set mode, 26-key input(k26) or 9-key input(k9).
 * @param obj  pointer to a Pinyin input method object
 * @param mode   the mode from 'lv_keyboard_mode_t'
 */
void lv_ime_pinyin_set_mode(lv_obj_t * obj, lv_ime_pinyin_mode_t mode)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    LV_ASSERT_OBJ(pinyin_ime->kb, &lv_keyboard_class);

    pinyin_ime->mode = mode;

#if LV_IME_PINYIN_USE_K9_MODE
    if(pinyin_ime->mode == LV_IME_PINYIN_MODE_K9) {
        pinyin_k9_init_data(obj);
        lv_keyboard_set_map(pinyin_ime->kb, LV_KEYBOARD_MODE_USER_1, (const char *)lv_btnm_def_pinyin_k9_map,
                            (const)default_kb_ctrl_k9_map);
        lv_keyboard_set_mode(pinyin_ime->kb, LV_KEYBOARD_MODE_USER_1);
    }
#endif
}

/*=====================
 * Getter functions
 *====================*/

/**
 * Set the dictionary of Pinyin input method.
 * @param obj  pointer to a Pinyin IME object
 * @return     pointer to the Pinyin IME keyboard
 */
lv_obj_t * lv_ime_pinyin_get_kb(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    return pinyin_ime->kb;
}

/**
 * Set the dictionary of Pinyin input method.
 * @param obj  pointer to a Pinyin input method object
 * @return     pointer to the Pinyin input method candidate panel
 */
lv_obj_t * lv_ime_pinyin_get_cand_panel(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    return pinyin_ime->cand_panel;
}

/**
 * Set the dictionary of Pinyin input method.
 * @param obj  pointer to a Pinyin input method object
 * @return     pointer to the Pinyin input method dictionary
 */
lv_pinyin_dict_t * lv_ime_pinyin_get_dict(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    return pinyin_ime->dict;
}

/*=====================
 * Other functions
 *====================*/

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_ime_pinyin_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    uint16_t py_str_i = 0;
    uint16_t btnm_i = 0;
    for(btnm_i = 0; btnm_i < (LV_IME_PINYIN_CAND_TEXT_NUM + 3); btnm_i++) {
        if(btnm_i == 0) {
            lv_btnm_def_pinyin_sel_map[btnm_i] = "<";
        }
        else if(btnm_i == (LV_IME_PINYIN_CAND_TEXT_NUM + 1)) {
            lv_btnm_def_pinyin_sel_map[btnm_i] = ">";
        }
        else if(btnm_i == (LV_IME_PINYIN_CAND_TEXT_NUM + 2)) {
            lv_btnm_def_pinyin_sel_map[btnm_i] = "";
        }
        else {
            lv_pinyin_cand_str[py_str_i][0] = ' ';
            lv_btnm_def_pinyin_sel_map[btnm_i] = lv_pinyin_cand_str[py_str_i];
            py_str_i++;
        }
    }

    pinyin_ime->mode = LV_IME_PINYIN_MODE_K26;
    pinyin_ime->py_page = 0;
    pinyin_ime->ta_count = 0;
    pinyin_ime->cand_num = 0;
    lv_memset_00(pinyin_ime->input_char, sizeof(pinyin_ime->input_char));
    lv_memset_00(pinyin_ime->py_num, sizeof(pinyin_ime->py_num));
    lv_memset_00(pinyin_ime->py_pos, sizeof(pinyin_ime->py_pos));

    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);

    lv_obj_set_size(obj, LV_PCT(100), LV_PCT(55));
    lv_obj_align(obj, LV_ALIGN_BOTTOM_MID, 0, 0);

#if LV_IME_PINYIN_USE_DEFAULT_DICT
    init_pinyin_dict(obj, lv_ime_pinyin_def_dict);
#endif

    /* Init pinyin_ime->cand_panel */
    pinyin_ime->cand_panel = lv_btnmatrix_create(lv_scr_act());
    lv_btnmatrix_set_map(pinyin_ime->cand_panel, (const char **)lv_btnm_def_pinyin_sel_map);
    lv_obj_set_size(pinyin_ime->cand_panel, LV_PCT(100), LV_PCT(5));
    lv_obj_add_flag(pinyin_ime->cand_panel, LV_OBJ_FLAG_HIDDEN);

    lv_btnmatrix_set_one_checked(pinyin_ime->cand_panel, true);
    lv_obj_clear_flag(pinyin_ime->cand_panel, LV_OBJ_FLAG_CLICK_FOCUSABLE);

    /* Set cand_panel style*/
    // Default style
    lv_obj_set_style_bg_opa(pinyin_ime->cand_panel, LV_OPA_0, 0);
    lv_obj_set_style_border_width(pinyin_ime->cand_panel, 0, 0);
    lv_obj_set_style_pad_all(pinyin_ime->cand_panel, 8, 0);
    lv_obj_set_style_pad_gap(pinyin_ime->cand_panel, 0, 0);
    lv_obj_set_style_radius(pinyin_ime->cand_panel, 0, 0);
    lv_obj_set_style_pad_gap(pinyin_ime->cand_panel, 0, 0);
    lv_obj_set_style_base_dir(pinyin_ime->cand_panel, LV_BASE_DIR_LTR, 0);

    // LV_PART_ITEMS style
    lv_obj_set_style_radius(pinyin_ime->cand_panel, 12, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(pinyin_ime->cand_panel, lv_color_white(), LV_PART_ITEMS);
    lv_obj_set_style_bg_opa(pinyin_ime->cand_panel, LV_OPA_0, LV_PART_ITEMS);
    lv_obj_set_style_shadow_opa(pinyin_ime->cand_panel, LV_OPA_0, LV_PART_ITEMS);

    // LV_PART_ITEMS | LV_STATE_PRESSED style
    lv_obj_set_style_bg_opa(pinyin_ime->cand_panel, LV_OPA_COVER, LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(pinyin_ime->cand_panel, lv_color_white(), LV_PART_ITEMS | LV_STATE_PRESSED);

    /* event handler */
    lv_obj_add_event_cb(pinyin_ime->cand_panel, lv_ime_pinyin_cand_panel_event, LV_EVENT_VALUE_CHANGED, obj);
    lv_obj_add_event_cb(obj, lv_ime_pinyin_style_change_event, LV_EVENT_STYLE_CHANGED, NULL);

#if LV_IME_PINYIN_USE_K9_MODE
    pinyin_ime->k9_input_str_len = 0;
    pinyin_ime->k9_py_ll_pos = 0;
    pinyin_ime->k9_legal_py_count = 0;
    lv_memset_00(pinyin_ime->k9_input_str, LV_IME_PINYIN_K9_MAX_INPUT);

    pinyin_k9_init_data(obj);

    _lv_ll_init(&(pinyin_ime->k9_legal_py_ll), sizeof(ime_pinyin_k9_py_str_t));
#endif
}


static void lv_ime_pinyin_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);

    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    if(lv_obj_is_valid(pinyin_ime->kb))
        lv_obj_del(pinyin_ime->kb);

    if(lv_obj_is_valid(pinyin_ime->cand_panel))
        lv_obj_del(pinyin_ime->cand_panel);
}


static void lv_ime_pinyin_kb_event(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * kb = lv_event_get_target(e);
    lv_obj_t * obj = lv_event_get_user_data(e);

    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

#if LV_IME_PINYIN_USE_K9_MODE
    static const char * k9_py_map[8] = {"abc", "def", "ghi", "jkl", "mno", "pqrs", "tuv", "wxyz"};
#endif

    if(code == LV_EVENT_VALUE_CHANGED) {
        uint16_t btn_id  = lv_btnmatrix_get_selected_btn(kb);
        if(btn_id == LV_BTNMATRIX_BTN_NONE) return;

        const char * txt = lv_btnmatrix_get_btn_text(kb, lv_btnmatrix_get_selected_btn(kb));
        if(txt == NULL) return;

#if LV_IME_PINYIN_USE_K9_MODE
        if(pinyin_ime->mode == LV_IME_PINYIN_MODE_K9) {
            lv_obj_t * ta = lv_keyboard_get_textarea(pinyin_ime->kb);
            uint16_t tmp_btn_str_len = strlen(pinyin_ime->input_char);
            if((btn_id >= 16) && (tmp_btn_str_len > 0) && (btn_id < (16 + LV_IME_PINYIN_K9_CAND_TEXT_NUM))) {
                tmp_btn_str_len = strlen(pinyin_ime->input_char);
                lv_memset_00(pinyin_ime->input_char, sizeof(pinyin_ime->input_char));
                strcat(pinyin_ime->input_char, txt);
                pinyin_input_proc(obj);

                for(int index = 0; index < (pinyin_ime->ta_count + tmp_btn_str_len); index++) {
                    lv_textarea_del_char(ta);
                }

                pinyin_ime->ta_count = tmp_btn_str_len;
                pinyin_ime->k9_input_str_len = tmp_btn_str_len;
                lv_textarea_add_text(ta, pinyin_ime->input_char);

                return;
            }
        }
#endif

        if(strcmp(txt, "Enter") == 0 || strcmp(txt, LV_SYMBOL_NEW_LINE) == 0) {
            pinyin_ime_clear_data(obj);
            lv_obj_add_flag(pinyin_ime->cand_panel, LV_OBJ_FLAG_HIDDEN);
        }
        else if(strcmp(txt, LV_SYMBOL_BACKSPACE) == 0) {
            // del input char
            if(pinyin_ime->ta_count > 0) {
                if(pinyin_ime->mode == LV_IME_PINYIN_MODE_K26)
                    pinyin_ime->input_char[pinyin_ime->ta_count - 1] = '\0';
#if LV_IME_PINYIN_USE_K9_MODE
                else
                    pinyin_ime->k9_input_str[pinyin_ime->ta_count - 1] = '\0';
#endif

                pinyin_ime->ta_count = pinyin_ime->ta_count - 1;
                if(pinyin_ime->ta_count <= 0) {
                    lv_obj_add_flag(pinyin_ime->cand_panel, LV_OBJ_FLAG_HIDDEN);
#if LV_IME_PINYIN_USE_K9_MODE
                    lv_memset_00(lv_pinyin_k9_cand_str, sizeof(lv_pinyin_k9_cand_str));
                    strcpy(lv_pinyin_k9_cand_str[LV_IME_PINYIN_K9_CAND_TEXT_NUM], LV_SYMBOL_RIGHT"\0");
                    strcpy(lv_pinyin_k9_cand_str[LV_IME_PINYIN_K9_CAND_TEXT_NUM + 1], "\0");
#endif
                }
                else if(pinyin_ime->mode == LV_IME_PINYIN_MODE_K26) {
                    pinyin_input_proc(obj);
                }
#if LV_IME_PINYIN_USE_K9_MODE
                else if(pinyin_ime->mode == LV_IME_PINYIN_MODE_K9) {
                    pinyin_ime->k9_input_str_len = strlen(pinyin_ime->input_char) - 1;
                    pinyin_k9_get_legal_py(obj, pinyin_ime->k9_input_str, k9_py_map);
                    pinyin_k9_fill_cand(obj);
                    pinyin_input_proc(obj);
                }
#endif
            }
        }
        else if((strcmp(txt, "ABC") == 0) || (strcmp(txt, "abc") == 0) || (strcmp(txt, "1#") == 0)) {
            pinyin_ime->ta_count = 0;
            lv_memset_00(pinyin_ime->input_char, sizeof(pinyin_ime->input_char));
            return;
        }
        else if(strcmp(txt, LV_SYMBOL_KEYBOARD) == 0) {
            if(pinyin_ime->mode == LV_IME_PINYIN_MODE_K26) {
                lv_ime_pinyin_set_mode(pinyin_ime, LV_IME_PINYIN_MODE_K9);
            }
            else {
                lv_ime_pinyin_set_mode(pinyin_ime, LV_IME_PINYIN_MODE_K26);
                lv_keyboard_set_mode(pinyin_ime->kb, LV_KEYBOARD_MODE_TEXT_LOWER);
            }
            pinyin_ime_clear_data(obj);
        }
        else if(strcmp(txt, LV_SYMBOL_OK) == 0) {
            pinyin_ime_clear_data(obj);
        }
        else if((pinyin_ime->mode == LV_IME_PINYIN_MODE_K26) && ((txt[0] >= 'a' && txt[0] <= 'z') || (txt[0] >= 'A' &&
                                                                                                      txt[0] <= 'Z'))) {
            strcat(pinyin_ime->input_char, txt);
            pinyin_input_proc(obj);
            pinyin_ime->ta_count++;
        }
#if LV_IME_PINYIN_USE_K9_MODE
        else if((pinyin_ime->mode == LV_IME_PINYIN_MODE_K9) && (txt[0] >= 'a' && txt[0] <= 'z')) {
            for(uint16_t i = 0; i < 8; i++) {
                if((strcmp(txt, k9_py_map[i]) == 0) || (strcmp(txt, "abc ") == 0)) {
                    if(strcmp(txt, "abc ") == 0)    pinyin_ime->k9_input_str_len += strlen(k9_py_map[i]) + 1;
                    else                            pinyin_ime->k9_input_str_len += strlen(k9_py_map[i]);
                    pinyin_ime->k9_input_str[pinyin_ime->ta_count] = 50 + i;

                    break;
                }
            }
            pinyin_k9_get_legal_py(obj, pinyin_ime->k9_input_str, k9_py_map);
            pinyin_k9_fill_cand(obj);
            pinyin_input_proc(obj);
        }
        else if(strcmp(txt, LV_SYMBOL_LEFT) == 0) {
            pinyin_k9_cand_page_proc(obj, 0);
        }
        else if(strcmp(txt, LV_SYMBOL_RIGHT) == 0) {
            pinyin_k9_cand_page_proc(obj, 1);
        }
#endif
    }
}


static void lv_ime_pinyin_cand_panel_event(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * cand_panel = lv_event_get_target(e);
    lv_obj_t * obj = (lv_obj_t *)lv_event_get_user_data(e);

    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    if(code == LV_EVENT_VALUE_CHANGED) {
        uint32_t id = lv_btnmatrix_get_selected_btn(cand_panel);
        if(id == 0) {
            pinyin_page_proc(obj, 0);
            return;
        }
        if(id == (LV_IME_PINYIN_CAND_TEXT_NUM + 1)) {
            pinyin_page_proc(obj, 1);
            return;
        }

        const char * txt = lv_btnmatrix_get_btn_text(cand_panel, id);
        lv_obj_t * ta = lv_keyboard_get_textarea(pinyin_ime->kb);
        uint16_t index = 0;
        for(index = 0; index < pinyin_ime->ta_count; index++)
            lv_textarea_del_char(ta);

        lv_textarea_add_text(ta, txt);

        pinyin_ime_clear_data(obj);
    }
}


static void pinyin_input_proc(lv_obj_t * obj)
{
    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    pinyin_ime->cand_str = pinyin_search_matching(obj, pinyin_ime->input_char, &pinyin_ime->cand_num);
    if(pinyin_ime->cand_str == NULL) {
        return;
    }

    pinyin_ime->py_page = 0;

    for(uint8_t i = 0; i < LV_IME_PINYIN_CAND_TEXT_NUM; i++) {
        memset(lv_pinyin_cand_str[i], 0x00, sizeof(lv_pinyin_cand_str[i]));
        lv_pinyin_cand_str[i][0] = ' ';
    }

    // fill buf
    for(uint8_t i = 0; (i < pinyin_ime->cand_num && i < LV_IME_PINYIN_CAND_TEXT_NUM); i++) {
        for(uint8_t j = 0; j < 3; j++) {
            lv_pinyin_cand_str[i][j] = pinyin_ime->cand_str[i * 3 + j];
        }
    }

    lv_obj_clear_flag(pinyin_ime->cand_panel, LV_OBJ_FLAG_HIDDEN);
}

static void pinyin_page_proc(lv_obj_t * obj, uint16_t dir)
{
    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;
    uint16_t page_num = pinyin_ime->cand_num / LV_IME_PINYIN_CAND_TEXT_NUM;
    uint16_t sur = pinyin_ime->cand_num % LV_IME_PINYIN_CAND_TEXT_NUM;

    if(dir == 0) {
        if(pinyin_ime->py_page) {
            pinyin_ime->py_page--;
        }
    }
    else {
        if(sur == 0) {
            page_num -= 1;
        }
        if(pinyin_ime->py_page < page_num) {
            pinyin_ime->py_page++;
        }
        else return;
    }

    for(uint8_t i = 0; i < LV_IME_PINYIN_CAND_TEXT_NUM; i++) {
        memset(lv_pinyin_cand_str[i], 0x00, sizeof(lv_pinyin_cand_str[i]));
        lv_pinyin_cand_str[i][0] = ' ';
    }

    // fill buf
    uint16_t offset = pinyin_ime->py_page * (3 * LV_IME_PINYIN_CAND_TEXT_NUM);
    for(uint8_t i = 0; (i < pinyin_ime->cand_num && i < LV_IME_PINYIN_CAND_TEXT_NUM); i++) {
        if((sur > 0) && (pinyin_ime->py_page == page_num)) {
            if(i > sur)
                break;
        }
        for(uint8_t j = 0; j < 3; j++) {
            lv_pinyin_cand_str[i][j] = pinyin_ime->cand_str[offset + (i * 3) + j];
        }
    }
}


static void lv_ime_pinyin_style_change_event(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);

    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    if(code == LV_EVENT_STYLE_CHANGED) {
        const lv_font_t * font = lv_obj_get_style_text_font(obj, LV_PART_MAIN);
        lv_obj_set_style_text_font(pinyin_ime->cand_panel, font, 0);
    }
}


static void init_pinyin_dict(lv_obj_t * obj, lv_pinyin_dict_t * dict)
{
    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    char headletter = 'a';
    uint16_t offset_sum = 0;
    uint16_t offset_count = 0;
    uint16_t letter_calc = 0;

    pinyin_ime->dict = dict;

    for(uint16_t i = 0; ; i++) {
        if((NULL == (dict[i].py)) || (NULL == (dict[i].py_mb))) {
            headletter = dict[i - 1].py[0];
            letter_calc = headletter - 'a';
            pinyin_ime->py_num[letter_calc] = offset_count;
            break;
        }

        if(headletter == (dict[i].py[0])) {
            offset_count++;
        }
        else {
          /* 先保存上一个字母的统计信息 */
          letter_calc = headletter - 'a';
          pinyin_ime->py_num[letter_calc] = offset_count;

          /* 更新总偏移量 */
          offset_sum += offset_count;

          /* 更新为新的首字母 */
          headletter = dict[i].py[0];
          letter_calc = headletter - 'a';

          /* 记录新字母在字典中的起始位置 */
          pinyin_ime->py_pos[letter_calc] = offset_sum;

          offset_count = 1;
        }
    }
}


static char * pinyin_search_matching(lv_obj_t * obj, char * py_str, uint16_t * cand_num)
{
    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    lv_pinyin_dict_t * cpHZ;
    uint8_t index, len = 0, offset;
    volatile uint8_t count = 0;

    if(*py_str == '\0')    return NULL;
    if(*py_str == 'i')     return NULL;
    if(*py_str == 'u')     return NULL;
    if(*py_str == 'v')     return NULL;

    offset = py_str[0] - 'a';
    len = strlen(py_str);

    cpHZ  = &pinyin_ime->dict[pinyin_ime->py_pos[offset]];
    count = pinyin_ime->py_num[offset];

    while(count--) {
        for(index = 0; index < len; index++) {
            if(*(py_str + index) != *((cpHZ->py) + index)) {
                break;
            }
        }

        // perfect match
        if(len == 1 || index == len) {
            // The Chinese character in UTF-8 encoding format is 3 bytes
            * cand_num = strlen((const char *)(cpHZ->py_mb)) / 3;
            return (char *)(cpHZ->py_mb);
        }
        cpHZ++;
    }
    return NULL;
}

static void pinyin_ime_clear_data(lv_obj_t * obj)
{
    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;


#if LV_IME_PINYIN_USE_K9_MODE
    if(pinyin_ime->mode == LV_IME_PINYIN_MODE_K9) {
        pinyin_ime->k9_input_str_len = 0;
        pinyin_ime->k9_py_ll_pos = 0;
        pinyin_ime->k9_legal_py_count = 0;
        lv_memset_00(pinyin_ime->k9_input_str,  LV_IME_PINYIN_K9_MAX_INPUT);
        lv_memset_00(lv_pinyin_k9_cand_str, sizeof(lv_pinyin_k9_cand_str));
        strcpy(lv_pinyin_k9_cand_str[LV_IME_PINYIN_K9_CAND_TEXT_NUM], LV_SYMBOL_RIGHT"\0");
        strcpy(lv_pinyin_k9_cand_str[LV_IME_PINYIN_K9_CAND_TEXT_NUM + 1], "\0");
    }
#endif

    pinyin_ime->ta_count = 0;
    lv_memset_00(lv_pinyin_cand_str, (sizeof(lv_pinyin_cand_str)));
    lv_memset_00(pinyin_ime->input_char, sizeof(pinyin_ime->input_char));

    lv_obj_add_flag(pinyin_ime->cand_panel, LV_OBJ_FLAG_HIDDEN);
}


#if LV_IME_PINYIN_USE_K9_MODE
static void pinyin_k9_init_data(lv_obj_t * obj)
{
    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    uint16_t py_str_i = 0;
    uint16_t btnm_i = 0;
    for(btnm_i = 19; btnm_i < (LV_IME_PINYIN_K9_CAND_TEXT_NUM + 21); btnm_i++) {
        if(py_str_i == LV_IME_PINYIN_K9_CAND_TEXT_NUM) {
            strcpy(lv_pinyin_k9_cand_str[py_str_i], LV_SYMBOL_RIGHT"\0");
        }
        else if(py_str_i == LV_IME_PINYIN_K9_CAND_TEXT_NUM + 1) {
            strcpy(lv_pinyin_k9_cand_str[py_str_i], "\0");
        }
        else {
            strcpy(lv_pinyin_k9_cand_str[py_str_i], " \0");
        }

        lv_btnm_def_pinyin_k9_map[btnm_i] = lv_pinyin_k9_cand_str[py_str_i];
        py_str_i++;
    }

    default_kb_ctrl_k9_map[0]  = LV_KEYBOARD_CTRL_BTN_FLAGS | 1;
    default_kb_ctrl_k9_map[4]  = LV_KEYBOARD_CTRL_BTN_FLAGS | 1;
    default_kb_ctrl_k9_map[5]  = LV_KEYBOARD_CTRL_BTN_FLAGS | 1;
    default_kb_ctrl_k9_map[9]  = LV_KEYBOARD_CTRL_BTN_FLAGS | 1;
    default_kb_ctrl_k9_map[10] = LV_KEYBOARD_CTRL_BTN_FLAGS | 1;
    default_kb_ctrl_k9_map[14] = LV_KEYBOARD_CTRL_BTN_FLAGS | 1;
    default_kb_ctrl_k9_map[15] = LV_KEYBOARD_CTRL_BTN_FLAGS | 1;
    default_kb_ctrl_k9_map[LV_IME_PINYIN_K9_CAND_TEXT_NUM + 16] = LV_KEYBOARD_CTRL_BTN_FLAGS | 1;
}

static void pinyin_k9_get_legal_py(lv_obj_t * obj, char * k9_input, const char * py9_map[])
{
    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    uint16_t len = strlen(k9_input);

    if((len == 0) || (len >= LV_IME_PINYIN_K9_MAX_INPUT)) {
        return;
    }

    char py_comp[LV_IME_PINYIN_K9_MAX_INPUT] = {0};
    int mark[LV_IME_PINYIN_K9_MAX_INPUT] = {0};
    int index = 0;
    int flag = 0;
    int count = 0;

    uint32_t ll_len = 0;
    ime_pinyin_k9_py_str_t * ll_index = NULL;

    ll_len = _lv_ll_get_len(&pinyin_ime->k9_legal_py_ll);
    ll_index = _lv_ll_get_head(&pinyin_ime->k9_legal_py_ll);

    while(index != -1) {
        if(index == len) {
            if(pinyin_k9_is_valid_py(obj, py_comp)) {
                if((count >= ll_len) || (ll_len == 0)) {
                    ll_index = _lv_ll_ins_tail(&pinyin_ime->k9_legal_py_ll);
                    strcpy(ll_index->py_str, py_comp);
                }
                else if((count < ll_len)) {
                    strcpy(ll_index->py_str, py_comp);
                    ll_index = _lv_ll_get_next(&pinyin_ime->k9_legal_py_ll, ll_index);
                }
                count++;
            }
            index--;
        }
        else {
            flag = mark[index];
            if(flag < strlen(py9_map[k9_input[index] - '2'])) {
                py_comp[index] = py9_map[k9_input[index] - '2'][flag];
                mark[index] = mark[index] + 1;
                index++;
            }
            else {
                mark[index] = 0;
                index--;
            }
        }
    }

    if(count > 0) {
        pinyin_ime->ta_count++;
        pinyin_ime->k9_legal_py_count = count;
    }
}


/*true: visible; false: not visible*/
static bool pinyin_k9_is_valid_py(lv_obj_t * obj, char * py_str)
{
    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    lv_pinyin_dict_t * cpHZ = NULL;
    uint8_t index = 0, len = 0, offset = 0;
    uint16_t ret = 1;
    volatile uint8_t count = 0;

    if(*py_str == '\0')    return false;
    if(*py_str == 'i')     return false;
    if(*py_str == 'u')     return false;
    if(*py_str == 'v')     return false;

    offset = py_str[0] - 'a';
    len = strlen(py_str);

    cpHZ  = &pinyin_ime->dict[pinyin_ime->py_pos[offset]];
    count = pinyin_ime->py_num[offset];

    while(count--) {
        for(index = 0; index < len; index++) {
            if(*(py_str + index) != *((cpHZ->py) + index)) {
                break;
            }
        }

        // perfect match
        if(len == 1 || index == len) {
            return true;
        }
        cpHZ++;
    }
    return false;
}


static void pinyin_k9_fill_cand(lv_obj_t * obj)
{
    static uint16_t len = 0;
    uint16_t index = 0, tmp_len = 0;
    ime_pinyin_k9_py_str_t * ll_index = NULL;

    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    tmp_len = pinyin_ime->k9_legal_py_count;

    if(tmp_len != len) {
        lv_memset_00(lv_pinyin_k9_cand_str, sizeof(lv_pinyin_k9_cand_str));
        strcpy(lv_pinyin_k9_cand_str[LV_IME_PINYIN_K9_CAND_TEXT_NUM], LV_SYMBOL_RIGHT"\0");
        strcpy(lv_pinyin_k9_cand_str[LV_IME_PINYIN_K9_CAND_TEXT_NUM + 1], "\0");
        len = tmp_len;
    }

    ll_index = _lv_ll_get_head(&pinyin_ime->k9_legal_py_ll);
    strcpy(pinyin_ime->input_char, ll_index->py_str);
    while(ll_index) {
        if((index >= LV_IME_PINYIN_K9_CAND_TEXT_NUM) || \
           (index >= pinyin_ime->k9_legal_py_count))
            break;

        strcpy(lv_pinyin_k9_cand_str[index], ll_index->py_str);
        ll_index = _lv_ll_get_next(&pinyin_ime->k9_legal_py_ll, ll_index); /*Find the next list*/
        index++;
    }
    pinyin_ime->k9_py_ll_pos = index;

    lv_obj_t * ta = lv_keyboard_get_textarea(pinyin_ime->kb);
    for(index = 0; index < pinyin_ime->k9_input_str_len; index++) {
        lv_textarea_del_char(ta);
    }
    pinyin_ime->k9_input_str_len = strlen(pinyin_ime->input_char);
    lv_textarea_add_text(ta, pinyin_ime->input_char);
}


static void pinyin_k9_cand_page_proc(lv_obj_t * obj, uint16_t dir)
{
    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    lv_obj_t * ta = lv_keyboard_get_textarea(pinyin_ime->kb);
    uint16_t ll_len =  _lv_ll_get_len(&pinyin_ime->k9_legal_py_ll);

    if((ll_len > LV_IME_PINYIN_K9_CAND_TEXT_NUM) && (pinyin_ime->k9_legal_py_count > LV_IME_PINYIN_K9_CAND_TEXT_NUM)) {
        ime_pinyin_k9_py_str_t * ll_index = NULL;
        uint16_t tmp_btn_str_len = 0;
        int count = 0;

        ll_index = _lv_ll_get_head(&pinyin_ime->k9_legal_py_ll);
        while(ll_index) {
            if(count >= pinyin_ime->k9_py_ll_pos)   break;

            ll_index = _lv_ll_get_next(&pinyin_ime->k9_legal_py_ll, ll_index); /*Find the next list*/
            count++;
        }

        if((NULL == ll_index) && (dir == 1))   return;

        lv_memset_00(lv_pinyin_k9_cand_str, sizeof(lv_pinyin_k9_cand_str));
        strcpy(lv_pinyin_k9_cand_str[LV_IME_PINYIN_K9_CAND_TEXT_NUM], LV_SYMBOL_RIGHT"\0");
        strcpy(lv_pinyin_k9_cand_str[LV_IME_PINYIN_K9_CAND_TEXT_NUM + 1], "\0");

        // next page
        if(dir == 1) {
            count = 0;
            while(ll_index) {
                if(count >= (LV_IME_PINYIN_K9_CAND_TEXT_NUM - 1))
                    break;

                strcpy(lv_pinyin_k9_cand_str[count], ll_index->py_str);
                ll_index = _lv_ll_get_next(&pinyin_ime->k9_legal_py_ll, ll_index); /*Find the next list*/
                count++;
            }
            pinyin_ime->k9_py_ll_pos += count - 1;

        }
        // previous page
        else {
            count = LV_IME_PINYIN_K9_CAND_TEXT_NUM - 1;
            ll_index = _lv_ll_get_prev(&pinyin_ime->k9_legal_py_ll, ll_index);
            while(ll_index) {
                if(count < 0)  break;

                strcpy(lv_pinyin_k9_cand_str[count], ll_index->py_str);
                ll_index = _lv_ll_get_prev(&pinyin_ime->k9_legal_py_ll, ll_index); /*Find the previous list*/
                count--;
            }

            if(pinyin_ime->k9_py_ll_pos > LV_IME_PINYIN_K9_CAND_TEXT_NUM)
                pinyin_ime->k9_py_ll_pos -= 1;
        }

        lv_textarea_set_cursor_pos(ta, LV_TEXTAREA_CURSOR_LAST);
    }
}

#endif  /*LV_IME_PINYIN_USE_K9_MODE*/

#endif  /*LV_USE_IME_PINYIN*/

