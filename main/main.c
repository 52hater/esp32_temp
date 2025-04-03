#include "main.h"

void app_main(void){
    // rtc는 시스템초기화에 가까우니까 main에, 그리고 가장먼저(하드웨어에서 시간을 가져와야 됨(다른 컴포넌트의 동작에 영향))
    // 근데 rtc 초기화 자체는 부팅과정에서 자동으로 수행되고,
    // 우리가 할 sntp_rtc 초기화는 단순히 인터넷에서 시간만 가져오는거라서 와이파이 연결 한 후에 하면 됨. 하드웨어에 영향X
    // sntp_rtc는 ap_main에 넣어두고 웹소켓메인 다음줄에 실행되도록 해놓음.

    hw_init();
    app_init();

    app_main();
}