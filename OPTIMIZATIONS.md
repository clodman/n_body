# Performance Optimizations

## Branch Elimination in N-Body Loop (2x speedup)

**Date:** 2026-02-17
**Branch:** `optimize`
**Commits:** 31beff7, 1fe38a4

### Change
Separated `balls_interact()` into two functions:
- `balls_accelerate()` - Gravity calculations only (branch-free)
- `balls_colide()` - Collision detection and resolution

### Why It's Faster

**Before:** Mixed gravity and collision in one loop
```c
for each ball pair:
    calculate distance
    if (collision) {        // ← Branch mispredictions!
        resolve_collision()
    }
    calculate gravity
```

**After:** Separate loops
```c
// Loop 1: Pure gravity (no branches)
for each ball pair:
    calculate distance
    apply gravity forces

// Loop 2: Collisions (branches isolated)
for each ball pair:
    if (collision) {
        resolve_collision()
    }
```

### Result
- **2x overall speedup**
- Scaled from 10 to 800 balls (80x more) at 120 FPS
- ~320,000 pair checks per frame with minimal branch mispredictions
- Gravity loop is now branch-free, allowing CPU pipeline optimization

### Key Insight
Branch mispredictions cost 10-20 CPU cycles. In tight O(N²) loops with unpredictable collision patterns, eliminating branches from the hot path far outweighs the cost of calculating distance twice.

### Secondary Changes
- Removed trajectory trails system (memory and performance)
- Extracted input handling to `balls_handle_input()`
