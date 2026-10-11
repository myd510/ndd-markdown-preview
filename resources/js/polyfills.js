/*
 * Chromium 77 兼容垫片（polyfill）
 * ------------------------------------------------------------
 * Qt 5.14.x 的 QtWebEngine 内核是 Chromium 77，缺少以下较新的运行时 API：
 *   String.prototype.replaceAll   (Chrome 85)
 *   Object.hasOwn                 (Chrome 93)
 *   Array/String.prototype.at     (Chrome 92)
 *   structuredClone               (Chrome 98)
 *   Element.prototype.replaceChildren (Chrome 86)
 *   Array.prototype.findLast      (Chrome 97)
 *   Promise.any                   (Chrome 85)
 * Mermaid / markdown-it 生态会用到它们，缺失时会直接抛错导致预览空白。
 * 本文件必须在 mermaid.min.js 与 preview.bundle.js 之前加载。
 *
 * 语法刻意保持保守（ES5/ES2015），确保在 Chromium 77 上可解析。
 */
(function () {
  'use strict';

  /* ---------- String.prototype.replaceAll ---------- */
  if (!String.prototype.replaceAll) {
    String.prototype.replaceAll = function (search, replacement) {
      var str = String(this);
      if (Object.prototype.toString.call(search) === '[object RegExp]') {
        if (!search.global) {
          throw new TypeError('replaceAll must be called with a global RegExp');
        }
        return str.replace(search, replacement);
      }
      var target = String(search);
      // 用 split/join 实现字面量全局替换，避免正则元字符问题
      return str.split(target).join(String(replacement));
    };
  }

  /* ---------- Array.prototype.at / String.prototype.at ---------- */
  if (!Array.prototype.at) {
    Array.prototype.at = function (n) {
      n = Math.trunc(n) || 0;
      if (n < 0) n += this.length;
      if (n < 0 || n >= this.length) return undefined;
      return this[n];
    };
  }
  if (!String.prototype.at) {
    String.prototype.at = function (n) {
      n = Math.trunc(n) || 0;
      if (n < 0) n += this.length;
      if (n < 0 || n >= this.length) return undefined;
      return this.charAt(n);
    };
  }

  /* ---------- Object.hasOwn ---------- */
  if (!Object.hasOwn) {
    Object.hasOwn = function (obj, prop) {
      if (obj === null || obj === undefined) {
        throw new TypeError('Cannot convert undefined or null to object');
      }
      return Object.prototype.hasOwnProperty.call(Object(obj), prop);
    };
  }

  /* ---------- Object.fromEntries ---------- */
  if (!Object.fromEntries) {
    Object.fromEntries = function (entries) {
      var out = {};
      if (entries === null || entries === undefined) return out;
      var it = entries[Symbol.iterator] ? entries[Symbol.iterator]() : entries;
      var step = it.next();
      while (!step.done) {
        var pair = step.value;
        out[pair[0]] = pair[1];
        step = it.next();
      }
      return out;
    };
  }

  /* ---------- Array.prototype.flat / flatMap ---------- */
  if (!Array.prototype.flat) {
    Array.prototype.flat = function (depth) {
      var d = depth === undefined ? 1 : Math.trunc(depth);
      return this.reduce(function (acc, val) {
        if (Array.isArray(val) && d > 0) {
          return acc.concat(val.flat(d - 1));
        }
        return acc.concat(val);
      }, []);
    };
  }
  if (!Array.prototype.flatMap) {
    Array.prototype.flatMap = function (fn, thisArg) {
      return this.map(fn, thisArg).flat(1);
    };
  }

  /* ---------- Array.prototype.findLast / findLastIndex ---------- */
  if (!Array.prototype.findLast) {
    Array.prototype.findLast = function (fn, thisArg) {
      for (var i = this.length - 1; i >= 0; i--) {
        if (fn.call(thisArg, this[i], i, this)) return this[i];
      }
      return undefined;
    };
  }
  if (!Array.prototype.findLastIndex) {
    Array.prototype.findLastIndex = function (fn, thisArg) {
      for (var i = this.length - 1; i >= 0; i--) {
        if (fn.call(thisArg, this[i], i, this)) return i;
      }
      return -1;
    };
  }

  /* ---------- structuredClone（常用类型子集） ---------- */
  if (typeof window.structuredClone !== 'function') {
    window.structuredClone = function (value) {
      return (function clone(v, seen) {
        if (v === null || typeof v !== 'object') return v;
        if (v instanceof Date) return new Date(v.getTime());
        if (v instanceof RegExp) return new RegExp(v.source, v.flags);
        if (v instanceof Map) {
          var m = new Map();
          seen.set(v, m);
          v.forEach(function (val, key) { m.set(clone(key, seen), clone(val, seen)); });
          return m;
        }
        if (v instanceof Set) {
          var s = new Set();
          seen.set(v, s);
          v.forEach(function (val) { s.add(clone(val, seen)); });
          return s;
        }
        if (v instanceof ArrayBuffer) return v.slice(0);
        if (ArrayBuffer.isView(v)) return new v.constructor(v.buffer.slice(0));
        if (Array.isArray(v)) {
          var arr = [];
          seen.set(v, arr);
          for (var i = 0; i < v.length; i++) arr[i] = clone(v[i], seen);
          return arr;
        }
        var out = {};
        seen.set(v, out);
        Object.keys(v).forEach(function (k) { out[k] = clone(v[k], seen); });
        return out;
      })(value, new Map());
    };
  }

  /* ---------- Element.prototype.replaceChildren / append ---------- */
  if (typeof Element !== 'undefined' && !Element.prototype.replaceChildren) {
    Element.prototype.replaceChildren = function () {
      while (this.firstChild) this.removeChild(this.firstChild);
      if (arguments.length) this.append.apply(this, arguments);
    };
  }
  if (typeof Element !== 'undefined' && !Element.prototype.append) {
    Element.prototype.append = function () {
      for (var i = 0; i < arguments.length; i++) {
        var n = arguments[i];
        this.appendChild(typeof n === 'string' ? document.createTextNode(n) : n);
      }
    };
  }

  /* ---------- Promise.allSettled / Promise.any ---------- */
  if (!Promise.allSettled) {
    Promise.allSettled = function (iterable) {
      var arr = Array.prototype.slice.call(iterable);
      return Promise.all(arr.map(function (p) {
        return Promise.resolve(p).then(
          function (value) { return { status: 'fulfilled', value: value }; },
          function (reason) { return { status: 'rejected', reason: reason }; }
        );
      }));
    };
  }
  if (!Promise.any) {
    Promise.any = function (iterable) {
      var arr = Array.prototype.slice.call(iterable);
      return new Promise(function (resolve, reject) {
        var pending = arr.length;
        if (!pending) {
          reject(new Error('All promises were rejected'));
          return;
        }
        var errors = [];
        arr.forEach(function (p, i) {
          Promise.resolve(p).then(resolve, function (e) {
            errors[i] = e;
            if (--pending === 0) {
              reject(new Error('All promises were rejected'));
            }
          });
        });
      });
    };
  }

  /* ---------- globalThis / queueMicrotask ---------- */
  if (typeof window.globalThis === 'undefined') {
    window.globalThis = window;
  }
  if (typeof window.queueMicrotask !== 'function') {
    window.queueMicrotask = function (cb) {
      Promise.resolve().then(cb).catch(function (e) {
        setTimeout(function () { throw e; }, 0);
      });
    };
  }
})();
