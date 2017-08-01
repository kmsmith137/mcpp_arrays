- Figure out how to get a "pretty" compile-time error message (using static_assert or some other mechanism)
  in the following situations:

    - invalid type T in check_dtype<T>
    - invalid type T in first rs_array<T> constructor

- conversion (rs_array<T> &) -> (rs_array<void> &)

- "ladder" classes rs_narray<T>, rs_ncarray<T>, rs_carray<T>

- ref(), pointer() operators?

- ss_array!

- shared_ptr<mcpp_reaper> could be replaced by:

    struct reaper_data {
       reaper_data **pprev, *next;
       mccp_reaper *reaper;
       void *freeme;
    };

  Note that the pprev,next,reaper fields are collectively
  equivalent to a shared_ptr<mcpp_reaper>.
  
  This would avoid the need to allocate a separate reaper struct,
  in the case where the memory has been allocated with malloc().
  
  Don't forget to think about thread-safety!

  Hmm, can this all be done with an exotic template specialization
  of std::shared_ptr?
