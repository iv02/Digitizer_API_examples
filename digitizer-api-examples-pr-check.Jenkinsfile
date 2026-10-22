String normalizeToken(String raw) {
    String value = (raw ?: '').trim()
    if (!value) {
        return ''
    }
    String lower = value.toLowerCase()
    if (lower in ['null', 'none', 'n/a', 'na']) {
        return ''
    }
    return lower.replaceAll(/[^a-z0-9@._-]+/, '')
}

String sanitizeChatId(String raw) {
    String value = (raw ?: '').trim()
    if (!value) {
        return ''
    }
    String lower = value.toLowerCase()
    if (lower in ['null', 'none', 'n/a', 'na']) {
        return ''
    }
    return value
}

List<String> collectAuthorKeys(def env) {
    LinkedHashSet<String> keys = new LinkedHashSet<>()
    [
        env.CHANGE_AUTHOR,
        env.CHANGE_AUTHOR_EMAIL,
        env.CHANGE_AUTHOR_DISPLAY_NAME,
        env.CHANGE_AUTHOR_FULL_NAME,
        env.ghprbPullAuthorLogin,
        env.ghprbPullAuthorEmail,
        env.ghprbActualCommitAuthor
    ].each { value ->
        if (value?.toString()?.trim()) {
            keys.add(value.toString())
        }
    }
    return keys as List<String>
}

String resolveTelegramChatId(String mappingRaw, String defaultChatId, List<String> authorKeys) {
    Map<String, String> mapping = [:]
    for (String rawPart : (mappingRaw ?: '').split(',')) {
        String part = rawPart.trim()
        if (!part || !part.contains(':')) {
            continue
        }
        String[] pair = part.split(':', 2)
        String key = normalizeToken(pair[0])
        String value = sanitizeChatId(pair[1])
        if (key && value) {
            mapping[key] = value
        }
    }

    for (String rawKey : authorKeys) {
        String key = normalizeToken(rawKey)
        if (!key) {
            continue
        }
        if (mapping.containsKey(key)) {
            return mapping[key]
        }
        if (key.contains('@')) {
            String login = key.split('@', 2)[0]
            if (login && mapping.containsKey(login)) {
                return mapping[login]
            }
        }
    }
    return sanitizeChatId(defaultChatId)
}

List<String> parseNotifyChatIds(String rawIds, String defaultChatId) {
    LinkedHashSet<String> ids = new LinkedHashSet<>()
    for (String rawId : (rawIds ?: '').split(',')) {
        String id = sanitizeChatId(rawId)
        if (id) {
            ids.add(id)
        }
    }
    String defaultId = sanitizeChatId(defaultChatId)
    if (defaultId) {
        ids.add(defaultId)
    }
    return ids as List<String>
}

String selectTelegramSticker(String status, def env) {
    String raw = status == 'success'
        ? (env.TELEGRAM_SUCCESS_STICKER_IDS ?: '').toString()
        : (env.TELEGRAM_FAILURE_STICKER_IDS ?: '').toString()
    List<String> stickers = raw.split(',').collect { it.trim() }.findAll { it }
    if (stickers.isEmpty()) {
        return ''
    }

    int buildNumber = 0
    try {
        buildNumber = Integer.parseInt((env.BUILD_NUMBER ?: '0').toString())
    } catch (Exception ignored) {
        buildNumber = 0
    }
    return stickers[Math.abs(buildNumber) % stickers.size()]
}

void sendTelegramNotification(def steps, def env, String stickerStatus, String msgText) {
    int apiStatus = steps.sh(returnStatus: true, script: '''#!/bin/bash
set -u

for attempt in 1 2 3; do
  if curl -fsSI --connect-timeout 10 --max-time 20 \
    --socks5-hostname 127.0.0.1:1080 https://api.telegram.org/ >/dev/null; then
    exit 0
  fi

  if [ "${attempt}" -lt 3 ]; then
    echo "WARN: Telegram API availability check failed (attempt ${attempt}/3); retrying in 60 seconds..."
    sleep 60
  fi
done

exit 1
''')
    if (apiStatus != 0) {
        steps.echo "WARN: Telegram API is unavailable through SOCKS5 proxy (exit=${apiStatus}); notification skipped."
        return
    }

    String fallbackChatId = '973194221'
    String defaultChatId = sanitizeChatId(env.DEFAULT_TELEGRAM_CHAT_ID ?: fallbackChatId)
    List<String> authorKeys = collectAuthorKeys(env)
    String authorChatId = resolveTelegramChatId(env.TELEGRAM_CHAT_MAP, defaultChatId, authorKeys)
    List<String> chatIds = parseNotifyChatIds("${authorChatId},${defaultChatId}", defaultChatId)
    if (chatIds.isEmpty()) {
        steps.echo 'WARN: Telegram notification skipped: no target chat IDs resolved.'
        return
    }

    String stickerId = selectTelegramSticker(stickerStatus, env)
    List<String> credentialIds = []
    for (String rawId : (env.TELEGRAM_CREDENTIAL_IDS ?: env.TELEGRAM_CREDENTIAL_ID ?: '').split(',')) {
        if (rawId?.trim()) {
            credentialIds.add(rawId.trim())
        }
    }

    def deliver = {
        steps.withEnv([
            "TG_CHAT_IDS=${chatIds.join(',')}",
            "TG_STICKER_ID=${stickerId}",
            "TG_MESSAGE=${msgText}"
        ]) {
            int sendStatus = steps.sh(returnStatus: true, script: '''#!/bin/bash
set -euo pipefail

if [ -z "${TG_TOKEN:-}" ]; then
  echo "ERROR: TG_TOKEN is empty"
  exit 1
fi

send_with_retry() {
  local endpoint="$1"
  shift
  local attempt

  for attempt in 1 2 3; do
    if curl -fsS --connect-timeout 5 --max-time 20 \
      --socks5-hostname 127.0.0.1:1080 -X POST "${endpoint}" "$@"; then
      return 0
    fi

    if [ "${attempt}" -lt 3 ]; then
      echo "WARN: Telegram request failed (attempt ${attempt}/3); retrying in 60 seconds..."
      sleep 60
    fi
  done
  return 1
}

send_failed=0
for chat_id in ${TG_CHAT_IDS//,/ }; do
  if [ -n "${TG_STICKER_ID:-}" ]; then
    send_with_retry "https://api.telegram.org/bot${TG_TOKEN}/sendSticker" \
      --data-urlencode "chat_id=${chat_id}" \
      --data-urlencode "sticker=${TG_STICKER_ID}" || send_failed=1
  fi

  send_with_retry "https://api.telegram.org/bot${TG_TOKEN}/sendMessage" \
    --data-urlencode "chat_id=${chat_id}" \
    --data-urlencode "text=${TG_MESSAGE}" || send_failed=1
done

exit "${send_failed}"
''')
            if (sendStatus != 0) {
                steps.echo "WARN: Telegram notification delivery had failures (exit=${sendStatus})."
            }
        }
    }

    for (String credentialId : credentialIds) {
        try {
            steps.withCredentials([[$class: 'StringBinding', credentialsId: credentialId, variable: 'TG_TOKEN']]) {
                deliver()
            }
            return
        } catch (Exception ex) {
            steps.echo "WARN: Telegram credential '${credentialId}' is unavailable: ${ex.getClass().getSimpleName()}"
        }
    }

    String envToken = (env.TELEGRAM_BOT_TOKEN ?: '').trim()
    if (envToken) {
        steps.withEnv(["TG_TOKEN=${envToken}"]) {
            deliver()
        }
        return
    }

    steps.echo "WARN: Telegram notification skipped: no usable token found (tried credentials: ${credentialIds.join(', ')})."
}

def msvc(String command) {
    bat """@call "%VS_VCVARS%" >NUL
if errorlevel 1 exit /b 1
${command}
if errorlevel 1 exit /b 1
"""
}

int msvcStatus(String command) {
    return bat(returnStatus: true, script: """@call "%VS_VCVARS%" >NUL
if errorlevel 1 exit /b 1
${command}
if errorlevel 1 exit /b 1
""")
}

pipeline {
    agent none

    options {
        timestamps()
        skipDefaultCheckout(true)
        timeout(time: 4, unit: 'HOURS')
    }

    environment {
        VM_AGENT_LABEL = 'UbuntuVm'
        WINDOWS_AGENT_LABEL = 'Windows10Vm'
        EXAMPLES_CI_ROOT = 'C:\\digitizer-api-examples-ci'
        LIBS_ROOT = 'C:\\libs'
        VS_VCVARS = 'C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat'
        CMAKE_GEN = 'Ninja Multi-Config'
        WINDOWS_JOBS = '2'
        CTEST_TIMEOUT_SEC = '600'
        DEFAULT_TELEGRAM_CHAT_ID = '973194221'
        TELEGRAM_CHAT_MAP = credentials('telegram_chat_map')
        TELEGRAM_SUCCESS_STICKER_IDS = credentials('telegram_success_sticker_ids')
        TELEGRAM_FAILURE_STICKER_IDS = credentials('telegram_failure_sticker_ids')
        TELEGRAM_CREDENTIAL_ID = 'telegram-digitizer-ci-bot-token'
        TELEGRAM_CREDENTIAL_IDS = 'telegram-digitizer-ci-bot-token,telegram-bot-token'
        TELEGRAM_BOT_TOKEN = ''
        NOTIFY_CHAT_ID = ''
        NOTIFY_CHAT_IDS = ''
    }

    stages {
        stage('Resolve notification chat') {
            agent any
            steps {
                script {
                    String fallbackChatId = '973194221'
                    String defaultChatId = sanitizeChatId(env.DEFAULT_TELEGRAM_CHAT_ID ?: fallbackChatId)
                    String resolvedChatId = resolveTelegramChatId(
                        env.TELEGRAM_CHAT_MAP,
                        defaultChatId,
                        collectAuthorKeys(env)
                    )
                    env.NOTIFY_CHAT_ID = sanitizeChatId(resolvedChatId) ?: fallbackChatId
                    env.NOTIFY_CHAT_IDS = parseNotifyChatIds(
                        "${env.NOTIFY_CHAT_ID},${defaultChatId}",
                        defaultChatId
                    ).join(',')
                }
            }
        }

        stage('Checkout examples') {
            agent { label "${VM_AGENT_LABEL}" }
            steps {
                checkout scm
            }
        }

        stage('Prepare Linux workspace') {
            agent { label "${VM_AGENT_LABEL}" }
            steps {
                sh '''#!/bin/bash
set -euo pipefail

CI_ROOT="${HOME}/digitizer-api-examples-ci"
REPO="${CI_ROOT}/Digitizer_API_examples"

rm -rf "${REPO}"
mkdir -p "${REPO}"
rsync -a --delete "${WORKSPACE}/" "${REPO}/"

if [ ! -f "${REPO}/CMakeLists.txt" ]; then
  echo "ERROR: examples CMakeLists.txt is missing after checkout."
  exit 1
fi
'''
            }
        }

        stage('Build and test on both agents') {
            parallel {
                stage('Linux Debug + Release') {
                    stages {
                        stage('Build examples on Linux') {
                            agent { label "${VM_AGENT_LABEL}" }
                            steps {
                                sh '''#!/bin/bash
set -euo pipefail

require_cmd() {
  command -v "$1" >/dev/null 2>&1 || { echo "ERROR: required command not found: $1"; exit 1; }
}

require_cmd cmake
require_cmd ctest
require_cmd rsync

CI_ROOT="${HOME}/digitizer-api-examples-ci"
REPO="${CI_ROOT}/Digitizer_API_examples"
LIBS="${HOME}/libs"
API_ROOT="${REPO}/digitizer-api/linux"
GEN="Unix Makefiles"
if command -v ninja >/dev/null 2>&1; then GEN="Ninja"; fi

JOBS="$(nproc)"
if [ "${JOBS}" -gt 4 ]; then JOBS=4; fi

cd "${REPO}"
rm -rf "${CI_ROOT}/_install/Digitizer_API_examples" build-Debug build-Release
mkdir -p "${CI_ROOT}/_install/Digitizer_API_examples"

for build_type in Debug Release; do
  lower="${build_type,,}"
  build_dir="build-${build_type}"
  install_prefix="${CI_ROOT}/_install/Digitizer_API_examples/linux/${build_type}"
  qt_prefix="${LIBS}/qt/${lower}"
  grpc_prefix="${LIBS}/grpc/${lower}"

  test -f "${qt_prefix}/lib/cmake/Qt6/Qt6Config.cmake"
  test -f "${grpc_prefix}/lib/cmake/protobuf/protobuf-config.cmake"
  test -d "${API_ROOT}/${build_type}"

  rm -rf "${build_dir}" "${install_prefix}"
  mkdir -p "${build_dir}" "${install_prefix}"

  cmake -S . -B "${build_dir}" -G "${GEN}" \
    -DCMAKE_BUILD_TYPE="${build_type}" \
    -DCMAKE_INSTALL_PREFIX="${install_prefix}" \
    -DCMAKE_PREFIX_PATH="${qt_prefix};${grpc_prefix}" \
    -DDIGITIZER_API_EXAMPLES_PREBUILD_LIBS_ROOT="${LIBS}" \
    -DDIGITIZER_API_EXAMPLES_API_ROOT="${API_ROOT}"

  if [ "${GEN}" = "Ninja" ]; then
    cmake --build "${build_dir}" --parallel "${JOBS}"
  else
    cmake --build "${build_dir}" -- -j"${JOBS}"
  fi
  cmake --install "${build_dir}"
done
'''
                            }
                            post {
                                failure {
                                    script {
                                        sendTelegramNotification(this, env, 'linux-build-failed', "Linux Digitizer_API_examples build failed\nPR: ${env.CHANGE_ID ?: 'n/a'}\nSource: ${env.CHANGE_BRANCH ?: env.BRANCH_NAME ?: 'n/a'}\nTarget: ${env.CHANGE_TARGET ?: 'n/a'}\nJob: ${env.JOB_NAME}\nBuild: #${env.BUILD_NUMBER}")
                                    }
                                }
                            }
                        }

                        stage('Run Linux tests') {
                            agent { label "${VM_AGENT_LABEL}" }
                            steps {
                                sh '''#!/bin/bash
set -euo pipefail

CI_ROOT="${HOME}/digitizer-api-examples-ci"
REPO="${CI_ROOT}/Digitizer_API_examples"
cd "${REPO}"
export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-offscreen}"

for build_type in Debug Release; do
  build_dir="${REPO}/build-${build_type}"
  test -f "${build_dir}/CTestTestfile.cmake"
  ctest --test-dir "${build_dir}" -N
  ctest --test-dir "${build_dir}" --output-on-failure --stop-on-failure \
    -j1 --timeout "${CTEST_TIMEOUT_SEC}" \
    --output-junit "${WORKSPACE}/ctest-linux-${build_type}.xml"
done
'''
                            }
                            post {
                                always {
                                    junit allowEmptyResults: true, testResults: 'ctest-linux-*.xml'
                                }
                                failure {
                                    script {
                                        sendTelegramNotification(this, env, 'linux-tests-failed', "Linux Digitizer_API_examples tests failed\nPR: ${env.CHANGE_ID ?: 'n/a'}\nSource: ${env.CHANGE_BRANCH ?: env.BRANCH_NAME ?: 'n/a'}\nTarget: ${env.CHANGE_TARGET ?: 'n/a'}\nJob: ${env.JOB_NAME}\nBuild: #${env.BUILD_NUMBER}")
                                    }
                                }
                            }
                        }
                    }
                }

                stage('Windows Debug + Release') {
                    stages {
                        stage('Build examples on Windows') {
                            agent { label "${WINDOWS_AGENT_LABEL}" }
                            steps {
                                script {
                                    ws("${env.EXAMPLES_CI_ROOT}\\Digitizer_API_examples") {
                                        deleteDir()
                                        checkout scm
                                        msvc '''@echo off
setlocal EnableExtensions EnableDelayedExpansion
set "REPO=%EXAMPLES_CI_ROOT%\\Digitizer_API_examples"
set "INSTALL_ROOT=%EXAMPLES_CI_ROOT%\\_install\\Digitizer_API_examples\\windows"
set "API_ROOT=%REPO%\\digitizer-api\\win64"
if not exist "%LIBS_ROOT%" exit /b 1
if not exist "%API_ROOT%" exit /b 1
cd /d "%REPO%"
if exist "%INSTALL_ROOT%" rmdir /s /q "%INSTALL_ROOT%"
if exist "build-Debug" rmdir /s /q "build-Debug"
if exist "build-Release" rmdir /s /q "build-Release"
mkdir "%INSTALL_ROOT%"

for %%C in (Debug Release) do (
  if /I "%%C"=="Debug" (set "LOWER=debug") else (set "LOWER=release")
  set "BUILD_DIR=build-%%C"
  set "INSTALL_PREFIX=%INSTALL_ROOT%\\%%C"
  set "QT_PREFIX=%LIBS_ROOT%\\qt\\!LOWER!"
  set "GRPC_PREFIX=%LIBS_ROOT%\\grpc\\!LOWER!"
  set "QT_TOOLS=!QT_PREFIX!"
  if not exist "!QT_PREFIX!\\lib\\cmake\\Qt6\\Qt6Config.cmake" exit /b 1
  if not exist "!GRPC_PREFIX!\\lib\\cmake\\protobuf\\protobuf-config.cmake" exit /b 1
  if not exist "!QT_TOOLS!\\bin\\moc.exe" exit /b 1
  if not exist "!QT_TOOLS!\\bin\\rcc.exe" exit /b 1
  if not exist "!QT_TOOLS!\\bin\\uic.exe" exit /b 1
  if not exist "!API_ROOT!\\%%C" exit /b 1
  set "PATH=!QT_TOOLS!\\bin;%PATH%"
  set "DEPS_PREFIXES=!QT_PREFIX!;!GRPC_PREFIX!"

  cmake -S . -B "!BUILD_DIR!" -G "%CMAKE_GEN%" ^
    -DCMAKE_BUILD_TYPE=%%C ^
    -DCMAKE_INSTALL_PREFIX="!INSTALL_PREFIX!" ^
    -DCMAKE_PREFIX_PATH="!DEPS_PREFIXES!" ^
    -DDIGITIZER_API_EXAMPLES_PREBUILD_LIBS_ROOT="%LIBS_ROOT%" ^
    -DDIGITIZER_API_EXAMPLES_API_ROOT="!API_ROOT!" ^
    -DQt6_DIR="!QT_PREFIX!\\lib\\cmake\\Qt6" ^
    -DQT_HOST_PATH="!QT_TOOLS!" ^
    -DCMAKE_AUTOMOC_EXECUTABLE="!QT_TOOLS!\\bin\\moc.exe" ^
    -DCMAKE_AUTORCC_EXECUTABLE="!QT_TOOLS!\\bin\\rcc.exe" ^
    -DCMAKE_AUTOUIC_EXECUTABLE="!QT_TOOLS!\\bin\\uic.exe" ^
    "-DCMAKE_CONFIGURATION_TYPES=%%C"
  if errorlevel 1 exit /b !ERRORLEVEL!
  cmake --build "!BUILD_DIR!" --config %%C --parallel %WINDOWS_JOBS%
  if errorlevel 1 exit /b !ERRORLEVEL!
  cmake --install "!BUILD_DIR!" --config %%C
  if errorlevel 1 exit /b !ERRORLEVEL!
)
endlocal
'''
                                    }
                                }
                            }
                            post {
                                failure {
                                    script {
                                        node("${env.VM_AGENT_LABEL}") {
                                            sendTelegramNotification(this, env, 'windows-build-failed', "Windows Digitizer_API_examples build failed\nPR: ${env.CHANGE_ID ?: 'n/a'}\nSource: ${env.CHANGE_BRANCH ?: env.BRANCH_NAME ?: 'n/a'}\nTarget: ${env.CHANGE_TARGET ?: 'n/a'}\nJob: ${env.JOB_NAME}\nBuild: #${env.BUILD_NUMBER}")
                                        }
                                    }
                                }
                            }
                        }

                        stage('Run Windows tests') {
                            agent { label "${WINDOWS_AGENT_LABEL}" }
                            steps {
                                script {
                                    ws("${env.EXAMPLES_CI_ROOT}\\Digitizer_API_examples") {
                                        int testStatus = msvcStatus '''@echo off
setlocal EnableExtensions EnableDelayedExpansion
set "REPO=%EXAMPLES_CI_ROOT%\\Digitizer_API_examples"
set "API_ROOT=%REPO%\\digitizer-api\\win64"
cd /d "%REPO%"
set "TEST_RC=0"
set "PATH=%LIBS_ROOT%\\qt\\debug\\bin;%LIBS_ROOT%\\qt\\release\\bin;%LIBS_ROOT%\\grpc\\debug\\bin;%LIBS_ROOT%\\grpc\\release\\bin;%PATH%"
for %%C in (Debug Release) do (
  if not exist "build-%%C\\CTestTestfile.cmake" (
    echo ERROR: test build tree not found: build-%%C
    set "TEST_RC=1"
  ) else (
    ctest --test-dir "build-%%C" -C %%C --output-on-failure --stop-on-failure -j1 --timeout %CTEST_TIMEOUT_SEC% --output-junit "%WORKSPACE%\\ctest-windows-%%C.xml"
    if errorlevel 1 set "TEST_RC=1"
  )
)
exit /b !TEST_RC!
'''
                                        junit allowEmptyResults: true, testResults: 'ctest-windows-*.xml'
                                        if (testStatus != 0) {
                                            error('Windows tests failed.')
                                        }
                                    }
                                }
                            }
                            post {
                                failure {
                                    script {
                                        node("${env.VM_AGENT_LABEL}") {
                                            sendTelegramNotification(this, env, 'windows-tests-failed', "Windows Digitizer_API_examples tests failed\nPR: ${env.CHANGE_ID ?: 'n/a'}\nSource: ${env.CHANGE_BRANCH ?: env.BRANCH_NAME ?: 'n/a'}\nTarget: ${env.CHANGE_TARGET ?: 'n/a'}\nJob: ${env.JOB_NAME}\nBuild: #${env.BUILD_NUMBER}")
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    post {
        success {
            script {
                node("${env.VM_AGENT_LABEL}") {
                    sendTelegramNotification(this, env, 'success', "Digitizer_API_examples build and tests passed\nPR: ${env.CHANGE_ID ?: 'n/a'}\nSource: ${env.CHANGE_BRANCH ?: env.BRANCH_NAME ?: 'n/a'}\nTarget: ${env.CHANGE_TARGET ?: 'n/a'}\nJob: ${env.JOB_NAME}\nBuild: #${env.BUILD_NUMBER}")
                }
            }
        }
    }
}
