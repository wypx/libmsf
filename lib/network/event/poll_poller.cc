
#if 0

    if (pset != NULL)
      if (pthread_sigmask(SIG_BLOCK, pset, NULL))
        abort();
    nfds = poll(loop->poll_fds, (nfds_t)loop->poll_fds_used, timeout);
    if (pset != NULL)
      if (pthread_sigmask(SIG_UNBLOCK, pset, NULL))
        abort();

#endif