#### implement method

- [qpcpp table.cpp](https://github.com/QuantumLeaps/qpcpp-examples/blob/a4becf08ed700750fce0bfea22a8968349330e55/posix-win32-cmake/dpp/table.cpp)
    - qpcpp 和 一般 actor model 的差異是它有 pub/sub (observer)，`AO_Table->POST(pe, this);`，所以其實 sender 是不會知道誰接走了message。
    - 在裡面，event 和 signal， 又是兩個東西。signal 就是一般說的event，這裡的event 更像 signal group。
- 基本上，現在event driven 都是以 actor model 為主的state machine，而不會是polling based 的state machine。
    - polling 的工作交給一些專門的底層，然後直接轉換為 event 之後再處理。
    - 甚至可以想像，如果底層有externa interupt 的 mcu 直接把 急停 trigger event 送到上位機。上位機也就不需要花很多時間去 polling 或是 context switch。
    - [Event-based vs polling - SinelaboreRT](https://www.sinelabore.de/doku.php/wiki/toolbox/event-based_versus_polling)
        - 我的 master402 是以polling based 為主，這個是PDO,sdo的天生限制。但轉換也不難，就是要克制不要開太多的thread 就是了，Run to completion scheduling 就重要了。
- 如果能夠logging event(message) 了話，對事後的debug有很大的好處。
    - 如果我確定要用actor model，那因為message 是透過actor system 傳送的，如果system可以直接記錄，會簡單很多，就像 ros bag。
    - [caf Log Output](https://actor-framework.readthedocs.io/en/latest/ConfiguringActorApplications.html#log-output)
        - CAF 雖然有logging，但是功能也算陽春，沒有針對性topic name 去filter 之類的。
        - 基本和printf 是一樣的 `log::io::warning("remote_lookup timed out");`，用處不大，用spd log 就能完全取代。
        - 我記得在 caf test 時，有顯示 message from & to node 的功能。當然printf 的log 也可以做到完全一樣的功能，就是sender 直接 printf 自己的ID，message, target，就可以了。
            - 或是 receiver 用 `self->current_sender()`。
        - `delegate` 可以先過水做filter 之類的動作才送到target actor。
    - 基本上 sobjectizer default 是沒有logger的，要自己實現。send 一份 message copy to logging actor，或是 save to file (printf)。

我們可以回憶一下，其實ros 有兩個記錄的方法，一個是一般的 `ros_info`, `ros_debug` 之類，類似printf。
另一個就是ros bag，可以錄下所有的 topic，之後甚至可以回放。是binary format。
我記得當時debug 出撞車的就是 `ros_info` 的方法，因為有需要改一下程式什麼的。我想說的是，一般的log lib 對debug 應該就夠了，不用一個特別神奇的東西。

- 其實 要在原本的 actor 附加 logging 是不難，就是把原本的send 再包一層，加入logger object 當新的 API 就好。
    - call send 時，就先開logger 看看 event 有沒有被 filter，沒有就call logging 去紀錄 sender, receiver, message，最後才真的call 底層的send。
- 選 actor model lib 來做是沒錯，但是我是蠻猶豫的，因為它會和整個lib 深深綁定。
    - 比如說，所有的模組都要 pass in specific actor (warn/error event, or other trigger)。那就要把這個actor 傳進去。
    - 假設我只使用 global 變數, singleton 之類不影響API？
    - 這裡顯現logging 時不靠特定object，而是純文字的好處。**這讓我決定把LOG 獨立出來，不要和event 弄在一起。**

但event 總歸是要被 machine main program 處理的。什麼樣的方法才能在不影響API的情況下達成要求?。

- global object impl e.g. logging, event destination
    - logging 雖然有lib，但要選lib
    - 我真正要的是解決 actor 強制 dependent 的問題。
    - [Is it a good practice to have logger as a singleton?](https://stackoverflow.com/questions/8472678/is-it-a-good-practice-to-have-logger-as-a-singleton)
        - 一般認為DI 還是比較好，但需要hack。
    - 我也是覺得DI 比較好，但是要把 actor 轉成一個通用界面很奇怪，因為actor 的 sending 可以用 self(也是actor) or sys actor(environment)。 
        - 而且api還是被強制更動。
        - 不論採用哪種方法都要解決這個問題。先看看so5 和 CAF API差異。so5 比較像 CAF 的 sys，沒有self object。相比之下，caf 需要 self才完整。而且兩者datatype 完全不同，注定要有template。
            - 而且因為API 不同，我要先用一的class 把兩者 api 調成一樣。講白一點，就是 **adapter pattern**。
    - singleton 的進化是 [Registry](https://martinfowler.com/eaaCatalog/registry.html) or factory
    - [Chapter 4. DI patterns](https://livebook.manning.com/book/dependency-injection-in-dot-net/chapter-4/)
        - 神書一本
        - ambient context
            - c++ 好像沒這個功能。這都蠻動態，或是runtime feature。
    - 這個問題牽涉到 cross cutting corner 和因此而生的 [Aspect orienting programming](http://bitdewy.github.io/blog/2013/10/20/aop/)
        - 問題是 C++ 對AOP沒有很好的工具。aspectC++ 甚至是發明了新語法。它算是有點依賴動態的特性。
- [Catching exception from worker thread in the main thread](https://stackoverflow.com/questions/25282620/catching-exception-from-worker-thread-in-the-main-thread)
    - try catch 是一個可能的方向
        - try catch 本身無法cross thred。但是透過 queue 和 `std::rethrow_exception` 可以。
    - 這其實顯示了一件事，queue 是 general 的 唯一傳遞event (message)方法。裡面的消息則可以千變萬化，甚至是轉來轉去 event, exception, actor message, CSP message...。
        - 那 queue 裡的東西是越是 basic c++ support 的，才能越general
            - 參考 linux message queue 也很不錯，是傳 struct 。
        - 轉換 e.g. MPSC queue 裡放 std::string 的 event name, source, target。 spawn 一個 strint event to message transition actor, 並spawn N個event sender，把queue 裡的 string 轉成message 後用sender 發出。
        - actor 和 csp(rust) 都是 mpsc, csp(go) is mpmc。
        - 我會覺得 imply multi datatype 的channel 還是比較好。簡化channel數目，但這只有 actor model 和 string/int(event num) channel 兩個選項。
            - `eventpp` by int
            - 我覺得 actor 太強烈。
- 其他還有 signal slot
    - [Sigslot, a signal-slot library](https://github.com/palacaze/sigslot)
        - signal 和 event 的定義蠻模糊的。
        - 從 api 來看，signal 是限定一個event 對應多個handler。
        - [event queue](https://github.com/wqking/eventpp/blob/master/doc/cn/tutorial_eventqueue.md) 是可以接收多個event，多的handler。或是說 signal 更簡化但specific 一點。
            - 如果只有一個event system。我覺得 event queue 是最能獨立的。
            - eventpp 也有的像 singal 的 `CallbackList`
            - 和適合轉換到其他 event system 的 `EventDispatcher`。
    - signal 的call 法又不太一樣，幾乎都是 `void(void)`，這顯示event 的傳送style 非常多變。
- 我也可以直接不把通訊的interface 統一，而是 由模組提供 handler e.g. `on_empty_seg_error_cb`, `on_emergency_btn_touch_cb`，要求user 自己pass function ptr 進來。

考慮web

- 以下是一個瘋狂的想法，如果把 planner seg combine 後的結果 segment & speed profile，透過通訊送到 realtime processor (mcu/dsp/cheap cortex A) (下稱RTP) ，由 RTP 上的 intp 進行插補，那麼 linux planner 這邊不需要ethercat，也不需要realtime 。如果需要一台linux對超多插補，這種方法可以解放linux被realtime佔去的限制，也能得到 RTP 的 realtime 和 low price。前提是
    1. segment 需要有協定 send across internet
    2. RTP 上發生的 event 能夠送到 linux planner. 
        - 這就是為什麼我會在這裡提到這個方案。event 系統至少要能和 web protocol 互相轉換。
    3. mcu插補的難度還是很高，有fpu但沒有simd(neon)，可能需要先插補完，把資料一次送過來。32bit\*3\*500=6kb(assume 500ms傳一次)，這資料量不小，而且這還是 binary情況下。
        - 但是對不需要插補同動的case，如果 e.g. pp mode 能轉成通訊event 是蠻好的，甚至是像RPC那樣。因為本來的PP mode 一樣是要一直polling ，去確定 state machine 有沒有成功。
- 另外，如果 io板 之類，不走canopen or ethercat 當memory mapping，直接把io的結果轉成event，可以讓上位機連polloing memory 都省掉。說到底，realtime 是為了 control loop 準備的, e.g. 用 CSV mode 自己做 CSP。單純 csp, csv mode 都是用 realtime 下命令就好。
- [HA MQTT Event](https://www.home-assistant.io/integrations/event.mqtt/)
    - 用 定義好的 property，去規定event 的設定。

emit event 的方法 summary。以下由複雜到簡單。

- 有source & target & event type
- 有 target & event type
    - event type 之間轉換需要 map，那怕是function/constructor。
    - 另外尷尬的點是，static event type (templated based) 能夠避免 event string 沒對上之類的問題。但是擴充，轉換 都會麻煩很多，這些都要在 compile time 算完。我是會覺得需要，因為error 一般程式不會觸發。wire up 也是組成program時，不太有unittest。
        - 顯然int, string 不行。
        - caf actor, sobjectizer, 都算有做到一部分。[event-system](https://github.com/FrancoisSestier/event-system). 一些 ECS lib (Entt)。
        - 各種IDL, e.g. ros, dds, grpc。送是有了，但有沒有人receive 就不好說。
- void function
    - 不需要指定event，call function動作，本身就是指定。
    - module 用到 n 個 event 就要 pass in n 個function。
        - 假設同一module event 本來就要pass 到不同地方，那不是壞事。或是不同module 同一event。
- 換一個角度，程式的最高目標就是重用，如果用 interface 能省多少？假設我用的是static event type。
    - 模組之間不太重用 error type，因為可能要知道error 從哪個 module 發出，或是等到上層邏輯層，才決定要不要視為一樣的error event。
    - 模組內的 event disaptch 機制應該相對相同。如果上層的event 也是 static event type ，那確實簡單，但如果是動態 e.g. http, RPC...，那就要手動的都轉一次。
        - 如果要避免這個問題，就要在一開始event define 時加一些條件，比如要求 define `name` property。這樣才能在有需要時拿到轉換用的素材。但是這是不現實的，因為在做module時不可能知道之後的 dynamic type name, id (int) 是什麼。修改原code 會違反OCP。
        - 所以只要有動態就是要全部手動 mapping，但這是動態自己的問題，還算能接受。而且能簡化一定，就是用 rtti 的 typeid mapping 到 evnet sys 裡動態的value。然後用這個map 寫 override function。
    - 當然如果是signal slot，就要全部重轉，但那不是我們的主要目標。
    - 如果是要對應到後來指定的的event。也是要重轉，這可能比較常需要。
    - 單個模組內，event 的數量夠多才能顯現出這個mapping 的優勢。
    - 參考
        - [events](https://github.com/MCGallaspy/events?tab=readme-ov-file#the-solution)
            - 這個其實不錯。某方面來看，其實也只是讓pass in的多個function 可以簡化為一個。`Foo` 可以當 interface。`EventDispatcher` 我不喜歡。`foo->dispatchEv(A_event);` 就好。
                - default 的動作，可以用functor 在 template 插入，這樣就不用implement。
                    - 這會造成 template implement 需要暴露，還是pass in function 比較好。
            - 某方面來說，我要的不是完整的event system，而是function mapper，或template style 的 strategy pattern。
                - event system 實在太多了，而且都是 framework 的底層機制。我要做的不是重新做一個，而是一個輕鬆彌平差異的工具。
        - 如果target event system 有要template `so_5::send<ping>(ponger_, 1000);` ，似乎無法避免tempalte。這樣template 會傳染，模組也要用template，畢竟functor 只能用template。除非手工指定。或是macro（魔法對付魔法）
            - 好像可以用 inherit 時，template 加上 functor。在 calling 端functor 付加上後，type 就能一樣。
        - 一樣可以把 update function pass in module。[entt](https://github.com/skypjack/entt?tab=readme-ov-file#code-example)

複雜的可以透過 closure, member... 去降轉。

證明type erasure 無法簡化event system。

反證 type erasure 做不到：assume 做的到，interface 有templated dispatch function e.g. `foo.dispatch<A_event>(a_event);`。還有將任意datatype，轉成目標static event 的 functor 要放進 foo，`foo.set???`，fptr的存放要unknown type，如果轉成base interface ptr or void ptr，functor 必需失去type info，就沒用了. 如果在 foo.dispatch 才放，一樣break，一樣需要外部的template。

正證：首先，static typed event 如果要簡化，必須要有template 把datatype 傳進來。function with template 不論何種方法，就會造成需要引入template。維持template 會造成永遠無法擺脫template，要有方法實現mapping。

打破 `function with template`。雖然這條是對的，但是我可以在傳進來的時候維持，傳進interface 後用其它的方式去implement type 的mapping 關係，就是用動態的map，e.g. 從 type id mapping 到 用functor with template 後產生的funtion。如此一來，`foo.dispatch<A_event>(a_event);` 就會變成查表的動作。[When is using 'typeid' the best solution?](https://stackoverflow.com/a/6753605)。但我要能夠iterate 所有的event type，一一加到 map 裡面，需且要存typed function。

另外，怎麼讓 function with template 產生function，如果不手動用template 把type pass in？ 必定是要一個iterate 所有 event type的方法，而且必須要在compile time (用 template)，而不是事後拿iterate map，否則type info 會消失，typeid 也無法還原static type。那就是在hierachy variadic template 展開時，就要自動去call template function，否則一但離開，template info 就沒了。可是只有constructor 能夠展開，costructor 又需要class 的template，所以又回到原點。

還是我要求implement 的 class 的 constructor ，手動call 一次有 function with template 的constructor，然後再把它的map copy 過來。
但是放入template 的functor 也必須是要填入type的，我沒辦法送入這個東西 `recall<Functor<???>, ParentEventLists>()`， 這個有 template template argument，但是在 partial specialization 的時候，好像一定要把template 展開。**這樣了話，直接失敗**。

有手動注冊是可行的，但簡化的意義就不大了，這不像call function 時傳入就好，而是要一開始就把所有的event type call一次。
就像純 template 方式能work，但做為一切的底層，直接讓template 散佈，可不行。

這是有代價的，RTTI(typeinfo) 才能在runtime 知道傳入的type，才能做mapping。rtti 有代價是大家都知道的。
那既然是reflection，static reflection 呢?能不能突破我的正證？

- runtime reflection
    - [RTTR](https://github.com/rttrorg/rttr)
        - 自己實現的rtti
    - reflect-cpp
        - focus on serial/deserial data format
- static reflection
    - [refl-cpp](https://github.com/veselink1/refl-cpp)
        - 好像和我想的不同

這些好像都不是檢查type，而且都要自己額外對structure加東西，還不如用 typeid, stl::typeinfo，至少API 會更乾淨。

RTTI

[ Binary sizes and RTTI ](https://www.reddit.com/r/cpp/comments/11exd9c/binary_sizes_and_rtti/) ，可以看到RTTI 對 binary size 的影響基本是微不足道。不使用它大部份是design pattern 的因素，不是任何種類的 performance。

std::any 也可以做到？ [std any use case](https://zh-blog.logan.tw/2021/10/31/cxx-17-std-any-usage/#id4) 就是用event 當例子，但這個method 需要callback 去處理context，所以沒節省到。

> 只要沒有用dynamic cast 和 typeid，就能加上 -fno-rtti flag
> 另外沒有exception 也能用 -fno-exception

以上這些鬼方法，implement 後，應該要藏在最底層的 base class，user 才能直接用，叫user implement 根本要死。但是一放在底層又變ambiguous。要把pointer cast 回它的最底層，才能正確的呼叫使用 map 的 dispatch function。原lib 也有 ` static_cast<EventListener<Event>*>(listener)->onEvent(evt);`。這就顯示出了，map based implement 方法和 inherit based 方法本來就不同，如何合併? 而且本來的inherit implement 就需要一個helper 去幫call。
可能的方法是，要有 helper class，先在最底層的function，找看看map 有沒有已經存值，沒有了話就 static cast 到對應的type，並且calling。
