<?php
require_once ROOT . '/inc/dat/Coursev2DB.inc';
require_once ROOT . '/inc/dat/YySchoolDB.inc';
require_once ROOT . '/inc/biz/Config.inc';
require_once ROOT . '/inc/biz/Config_wechat.inc';
require_once ROOT . '/inc/lib/ValidateFiled.inc';
require_once ROOT . '/inc/lib/myFunc.inc';
require_once ROOT . '/inc/lib/ScoreSort.inc';
require_once ROOT . '/inc/dat/CourseDB.inc';

class ExpressSetting {

    protected $courseDb;
    protected $expressUrl = 'http://poll.kuaidi100.com/poll/query.do';
    //测试公众号
    protected $wxappid = 'wx27bfd4e69838ae16';  //微信 测试appid
    protected $wxsecret = '32716292b11f2210a7a253265f2418ef';//微信测试
    //三个品牌对应appid和secrete
    protected $wxappid_ds = 'wxb1842bafa082a1d1';  //微信 大山外语
    protected $wxsecret_ds = '82966dd0ba41b94fb086e0386b9caedf';//微信大山外语
    protected $wxappid_xsd = 'wx88175c7d86143b50';  //微信  小数点
    protected $wxsecret_xsd = '058688ff35bccb8d5a7ac536999d137f';//小数点
    protected $wxappid_yfz = 'wx5abb3a6b8ca1c0a7';  //大语文
    protected $wxsecret_yfz = 'd7800fad91c600e8e61f481d9f6c5d72';//大语文
    //推送模板id
//    protected $template_arr =[
//        1=>"0-zzkBKqLJ3q8hbxF-BGLmgMWqLotlZumyibO_EDud8", //报名视频课成功
//        2=>'3xwbOTcojnhWTuRuBnjFVRZTE4hI0vFXtmSYZZ4u4Y0',//报名直播课成功
//        3=>'3xwbOTcojnhWTuRuBnjFVRZTE4hI0vFXtmSYZZ4u4Y0',//直播开课前提醒
//        4=>'3xwbOTcojnhWTuRuBnjFVRZTE4hI0vFXtmSYZZ4u4Y0'  //直播修改时间提醒
//    ];
    //$course_type 1外语 2数学 3语文
    protected $template_arr =[
        1=>[
            'tx'=>'2VPbi2rVCveQ1SkzQbqdKzmPaV8TuE7HbzzDSfICl3Q',
            'tz'=>'E6BGmxXwMav4fiJXuDLSL0-VmHxTPIDW3axau1IszNs'
            ],
        2=>[
            'tx'=>'S_op6MzRYoIR26vOuOkr_dr3B_o1O2fo1vivqrburnQ',
            'tz'=>'SJyKpTNL9WrZATQU_pBa9XIjNhRXaCT5zvh3MaQyW8c'
            ],
        3=>[
            'tx'=>'M3FtfnqDBQhIJvFOJBGVT4sHIQneEBlHznGSU_XzF4M',
            'tz'=>'YtJii3oGL6wPkEmrz0F822UkS2Ov3VA2dU2M_Y5dq8U'
            ]
    ];
    protected $teacherModel;
    protected $validateObj;
    protected $studentObj;
    protected $userObj;

    public function __construct()
    {
//        define(TOKEN,'lwxtoken');
        $this->validateObj   = new ValidateFiled();
        $this->studentObj    = new Studentv2DB();
        $this->courseDb        = new Coursev2DB();
    }
    //学生购买视频直播课推送
    //student_id  course_type 1直播 2视频 course_id  t
    //type 1视频课购买 2直播课购买 3上课20分钟前 4直播修改时间

    public function studentBuyOnlineCourseMsg($datas){
//        $datas['student_id']=151906;
//        $datas['course_type']=1;
//        $datas['type']=1;
//        $datas['course_id']=1088;
        if($datas['course_type']==1){ // 直播课
            $courseinfo=CourseDB::get_table_valueone("plaso_live","id={$datas['course_id']}",'subject_id,name as course_name,teacher_id');
        }else{
            $courseinfo=CourseDB::get_table_valueone("course_tem","id={$datas['course_id']}",'subject_id,course_name,teacher_id');
        }
        if(!$courseinfo['subject_id']){ //无科目不推送
            $res['msg']='课程没科目';
            $res['status']=1;
            $res['data']=$datas;
            $res['data']['time']=date('Y-m-d H:i:s',time());
            return $res;
        }
        //获取用户openid
        $userinfo=CourseDB::get_table_valueone("wechat_users","student_id=".$datas['student_id'],"id,ds_openid,xsd_openid,yfz_openid");
        //判断品牌类型
        $pinpai=Config::$kemu_video_class;
        if(in_array($courseinfo['subject_id'],$pinpai['wen'])){  //语文
            $course_type=3;
            $openid=$userinfo['ds_openid'];
        }elseif(in_array($courseinfo['subject_id'],$pinpai['li'])){//数学
            $course_type=2;
            $openid=$userinfo['xsd_openid'];
        }elseif(in_array($courseinfo['subject_id'],$pinpai['yw'])){//英语
            $course_type=1;
            $openid=$userinfo['yfz_openid'];
        }else{
            $res['msg']='科目没对应品牌';
            $res['status']=2;
            $res['data']=$datas;
            $res['data']['time']=date('Y-m-d H:i:s',time());
//            $resencode=json_encode($res).'\r\n';
//            file_put_contents('wxtuisong.txt',$resencode,FILE_APPEND);
            return $res;
        }
        if(empty($openid)){
            $res['msg']='学生暂未关联对应品牌公众号';
            $res['status']=3;
            $res['data']=$datas;
            $res['data']['time']=date('Y-m-d H:i:s',time());
            return $res;
        }
        //获取token
        $token = self::actionGetToken($course_type);
        $tuisong=self::wxPushMsg($token,$datas['type'],$datas['course_id'],$courseinfo['course_name'],$openid,$datas['student_id'],$courseinfo['teacher_id'],$course_type);
        file_put_contents('1111111.txt',$tuisong);
        $result=json_decode($tuisong,true);
        if($result['errcode']==0){
            $res['msg']='推送成功';
            $res['status']=0;
            $res['data']=$result;
            return $res;
        }else{
            $res['msg']='推送失败';
            $res['status']=4;
            $res['data']=$result;
            $res['data']=$result;
            return $res;
        }
    }

    //type 1视频课购买 2直播课购买 3上课20分钟前 4直播修改时间
    //$course_type 1外语 2数学 3语文
    public function wxPushMsg($token,$type,$course_id,$coursename,$openid,$studentid,$teacherid,$course_type){

        $studentInfo=CourseDB::get_table_value_yyone('student',"id={$studentid}","id,true_name");
        $teacherinfo=CourseDB::get_table_valueone('user',"id={$teacherid}","id,true_name");
        //根据type获取推送内容
        if($type!=1){
            $courseinfo=CourseDB::get_table_valueone("plaso_live_chapter","plaso_live_id={$course_id} and has_play=0",'id,start_time,end_time','','`order` asc');
            $time=date('Y-m-d H:i',$courseinfo['start_time'])."-".date('H:i',$courseinfo['end_time']);
        }
        $firstarr=[
            1=>$studentInfo['true_name'].'同学你好，你已成功报名该课程！',
            2=>$studentInfo['true_name'].'同学你好，你已成功报名该课程！',
            3=>$studentInfo['true_name'].'同学你好，你已成功报名该课程！',
            4=>$studentInfo['true_name'].'同学你好，你报名的课程上课时间已调整！'
        ];
        $remarkarr=[
            1=>"点击详情，立即开始学习",
            2=>"点击查看课程详情",
            3=>"点击详情，立即进入直播间",
            4=>"点击查看课程详情"
        ];
        $keyword1="《{$coursename}》";
        $keyword2=$teacherinfo['true_name'];
        $keyword3=$time;
        $first=$firstarr[$type];
        $remark=$remarkarr[$type];

        $xcxappid=Config_wechat::$mini_program['default']['app_id']; //营业厅小程序appid
        //推送url
        $url = 'https://api.weixin.qq.com/cgi-bin/message/template/send?access_token='.$token;

        if($type==1){
            $xcxurl="pages/videodetail/videodetail?course_id=".$course_id;  //小程序跳转页面 视频
        }else{
            $xcxurl="pages/livedetail/livedetail?course_id=".$course_id;  //小程序跳转页面 直播
        }

        if($type==1){ //视频课提醒
            $template_id=$this->template_arr[$course_type]['tx'];  //模板id
        }else{
            $template_id=$this->template_arr[$course_type]['tz'];  //模板id
        }
//        $template_id=$this->template_arr[$type];  //模板id
        //模板修改为通用模板
        if($type==1){
            $data=[
                'first'=>['value'=>$first,'color'=>'#173177'],
                'keyword1'=>['value'=>$studentInfo['true_name'],'color'=>'#173177'],  //学生名
                'keyword2'=>['value'=>$keyword1,'color'=>'#173177'],  //课程名
                'keyword3'=>['value'=>$keyword2,'color'=>'#173177'],//老师名
                'remark'=>['value'=>$remark,'color'=>'#173177'],
            ];
        }else{
            $data=[
                'first'=>['value'=>$first,'color'=>'#173177'],
                'keyword1'=>['value'=>$keyword1,'color'=>'#173177'],//课程名
                'keyword2'=>['value'=>$keyword3,'color'=>'#173177'],//时间
                'keyword3'=>['value'=>$keyword2,'color'=>'#173177'],//老师名
                'remark'=>['value'=>$remark,'color'=>'#173177'],
            ];
        }
        $message = [
            'touser'=>$openid,
            'template_id'=>$template_id,
//           'url'=>$cz_url,
            'miniprogram'=>[
                "appid"=>$xcxappid,
                "pagepath"=>$xcxurl,
            ],
            'data'=>$data,
        ];
        $data = json_encode($message);
        //发送
        $res = self::actionCurlRequest($url,$data);
        return $res;
    }
    //获取token
    public function actionGetToken($course_type){
        if($course_type==1){  //语文
            $openid=$this->wxappid_yfz;
            $secret=$this->wxsecret_yfz;
        }elseif($course_type==2){ //数学
            $openid=$this->wxappid_xsd;
            $secret=$this->wxsecret_xsd;
        }else{//英语
            $openid=$this->wxappid_ds;
            $secret=$this->wxsecret_ds;
        }
        $token=CourseDB::get_table_valueone("wx_config","appid='{$openid}' and appsecret='{$secret}'","access_token,expire_time");
        if($token['expire_time']>time()){
            $access_token=$token['access_token'];
            return $access_token;
        }
        $url = 'https://api.weixin.qq.com/cgi-bin/token?grant_type=client_credential&appid='.$openid.'&secret='.$secret;
        $res = json_decode(self::actionCurlRequest($url),true);
        $update['access_token']=$res['access_token'];
        $update['expire_time']=time()+7200;
        $updateres=CourseDB::update_table_value("wx_config",$update,"appid='{$openid}' and appsecret='{$secret}'");
        return $res['access_token'];
    }
    
    public function userJoin($params){
        if($params['type']==1){
            $re['appid']=$this->wxappid_yfz;
            $where ="yfz_openid='{$_SESSION['openid']}'";//语文
        }elseif($params['type']==2){
            $re['appid']=$this->wxappid_xsd;
            $where ="xsd_openid='{$_SESSION['openid']}'";//数学
        }elseif($params['type']==3){
            $re['appid']=$this->wxappid_ds;
            $where ="ds_openid='{$_SESSION['openid']}'"; //外语
        }
        $re['status']=0;//没关联
        $re['openid']='';

        if($_SESSION['openid']){
            $re['openid']=$_SESSION['openid'];
            $info=CourseDB::get_table_valueone('wechat_users',$where,'id');
            if($info){
                $re['status']=1;//已关联
            }
        }
        return $re;
    }
    public function getUserOpenid($params){
        if($params['type']==1){
            $appid=$this->wxappid_yfz;
            $secreate=$this->wxsecret_yfz;
        }elseif($params['type']==2){
            $appid=$this->wxappid_xsd;
            $secreate=$this->wxsecret_xsd;
        }elseif($params['type']==3){
            $appid=$this->wxappid_ds;
            $secreate=$this->wxsecret_ds;
        }

        $api='https://api.weixin.qq.com/sns/oauth2/access_token?appid='.$appid.'&secret='.$secreate.'&code='.$params['code'].'&grant_type=authorization_code';
        $result=file_get_contents($api);
        file_put_contents('111op.txt',$result);
        $res=json_decode($result,true);
        $openid=$res['openid'];  //获取到用户openid
        if($params['type']==1){
            $where="yfz_openid='".$openid."'";
        }elseif($params['type']==2){
            $where="xsd_openid='".$openid."'";
        }elseif($params['type']==3){
            $where="ds_openid='".$openid."'";
        }
        $userinfo=CourseDB::get_table_valueone("wechat_users ",$where,"id");
        if($userinfo['id']){
            $data['code']='1';//已经关联
            $data['openid']=$openid;//
            $_SESSION['openid']=$openid;
        }else{
            $data['code']='0';//未关联
            $data['openid']=$openid;//
            $_SESSION['openid']=$openid;
        }
        return $data;
    }
    //绑定大山外语
    public function userJoinWechat($params){
        $phone=$params['phone'];
        $pass= md5($params['pass'] . 'dszx');
        $openid=$params['openid'];
        $type=$params['type'];//1语文 2数学 3英语
        $user=CourseDB::get_table_value_yyone("student","u_phone=".$phone,"id");
        if(empty($user)){
            $data['status']=1;
            $data['msg']="该手机号没在学习8注册";
            return $data;
        }
        $userinfo=CourseDB::get_table_value_yyone("student","u_phone='{$phone}' and u_password='{$pass}'","id");
        if(empty($userinfo)){
            $data['status']=2;
            $data['msg']="密码错误";
            return $data;
        }
        file_put_contents('we11111.txt',$userinfo['id']);
        if($type==1){
            $update_data['yfz_openid']=$add['yfz_openid']=$openid;
        }elseif($type==2){
            $update_data['xsd_openid']=$add['xsd_openid']=$openid;
        }elseif($type==3){
            $update_data['ds_openid']=$add['ds_openid']=$openid;
        }
        file_put_contents('we1222222.txt',$openid);
        $wechatuser=CourseDB::get_table_valueone("wechat_users","student_id=".$userinfo['id'],"id");
        if($wechatuser){
            file_put_contents('we555.txt',json_encode($wechatuser));
            $update=CourseDB::update_table_value('wechat_users',$update_data,"student_id=".$userinfo['id']);
        }else{
            $add['student_id']=$userinfo['id'];
            file_put_contents('we3333.txt',json_encode($add));
            $update=CourseDB::addTableValue("wechat_users",$add);
            file_put_contents('we4444.txt',json_encode($update));
        }

        if($update){
            $data['status']=0;
            $data['msg']="绑定成功";
            return $data;
        }else{
            $data['status']=3;
            $data['msg']="绑定失败";
            return $data;
        }
    }
    public function wx(){
        file_put_contents('11token.txt',TOKEN);
        $signature = $_GET["signature"];
        $timestamp = $_GET["timestamp"];
        $nonce = $_GET["nonce"];
        file_put_contents('11NOCE.txt',$nonce);

        $token = TOKEN;
        $tmpArr = array($token, $timestamp, $nonce);
        sort($tmpArr, SORT_STRING);
        $tmpStr = implode( $tmpArr );
        $tmpStr = sha1( $tmpStr );

        if( $tmpStr == $signature ){
            file_put_contents('11TRUE.txt',$_GET["echostr"]);
            return $_GET["echostr"];
        }else{
            file_put_contents('11FALSE.txt','222');
            return false;
        }
    }

    //获取快递详细信息
    /*
     * return status 1:错误 0成功
     * return msg：提示信息
     * return data：返回数据
     * */
    public  function  getExpressInfo($datas)
    {
//        $datas['express_code']='yuantong';
//        $datas['express_num']='YT4535789291234';

        $bool = $this->validateObj->rules(['express_code'=>'isNotEmpty','express_num'=>'isNotEmpty'])->check($datas);
        $res['msg']     = '';
        $res['status']  = '1';
        if (!$bool){
            $res['msg'] = '参数错误';
            return $res;
        }
        //参数设置
        $key =  Config::$express100['key'];	//客户授权key
        $customer = Config::$express100['customer'];//查询公司编号
        $param = array (
            'com' => $datas['express_code'],			//快递公司编码
            'num' => $datas['express_num'],	//快递单号
            'phone' => '',				//手机号
            'from' => '',				//出发地城市
            'to' => '',					//目的地城市
            'resultv2' => '1'			//开启行政区域解析
        );
        //请求参数
        $post_data = array();
        $post_data["customer"] = $customer;
        $post_data["param"] = json_encode($param);
        $sign = md5($post_data["param"].$key.$post_data["customer"]);
        $post_data["sign"] = strtoupper($sign);
        $url =$this->expressUrl;	//实时查询请求地址
        $params = "";
        foreach ($post_data as $k=>$v) {
            $params .= "$k=".urlencode($v)."&";		//默认UTF-8编码格式
        }
        $post_str = substr($params, 0, -1);

        $post['method']='post';
        $post['url']=$url;
        $post['data']=$post_str;
        $result=curl_browser($post);
        $data=json_decode($result,true);

        foreach($data['data'] as $k=>$v){
            $timestr=strtotime($v['time']);
            $data['data'][$k]['timeHour']=date('h:i',$timestr);
            $data['data'][$k]['timeDay']=date('m月d日',$timestr);
            unset($data['data'][$k]['ftime']);
            unset($data['data'][$k]['ftime']);
            unset($data['data'][$k]['areaCode']);
        }
        if($data['status']==200){//成功
            $res['status']=0;
            $res['msg']=$data['message'];
            $res['data']=$data['data'];
        }elseif(isset($data['status'])&&!empty($data['status'])){
            $res['status']=$data['returnCode'];
            $res['msg']=$data['message'];
        }else{
            $res['status']  = '2';
            $res['msg'] = '请求接口失败';
        }
        return $res;
    }


//curl请求，支持post和get
    public function actionCurlRequest($url,$data=null){
        $curl = curl_init();
        curl_setopt($curl,CURLOPT_URL,$url);
        curl_setopt($curl,CURLOPT_SSL_VERIFYPEER,FALSE);
        curl_setopt($curl,CURLOPT_SSL_VERIFYHOST,FALSE);
        if(!empty($data)){
            curl_setopt($curl,CURLOPT_POST,1);
            curl_setopt($curl,CURLOPT_POSTFIELDS,$data);
        }

        curl_setopt($curl,CURLOPT_RETURNTRANSFER,1);
        $output = curl_exec($curl);
        curl_close($curl);
        return $output;
    }


}
