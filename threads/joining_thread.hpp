#include <thread>

namespace ext
{
    template <typename T1, typename T2>
    constexpr bool is_similar_v = std::is_same<std::decay_t<T1>, std::decay_t<T2>>::value;

    class JoiningThread
    {
        std::thread thd_;

    public:
        JoiningThread() = default;

        template <typename Function, typename... Args, typename = std::enable_if_t<!is_similar_v<Function, JoiningThread>>>
        JoiningThread(Function&& fn, Args&&... args)
            : thd_ {std::forward<Function>(fn), std::forward<Args>(args)...}
        {
        }

        JoiningThread(const JoiningThread&) = delete;
        JoiningThread& operator=(const JoiningThread&) = delete;
        JoiningThread(JoiningThread&&) = default;
        JoiningThread& operator=(JoiningThread&&) = default;

        ~JoiningThread()
        {
            if (thd_.joinable())
                thd_.join();
        }

        void join()
        {
            thd_.join();
        }

        void detach()
        {
            thd_.detach();
        }

        std::thread::id get_id() const
        {
            return thd_.get_id();
        }

        std::thread::native_handle_type native_handle()
        {
            return thd_.native_handle();
        }        
    };
}