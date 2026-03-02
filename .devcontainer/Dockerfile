# Use the existing eclipse-score devcontainer image
FROM ghcr.io/eclipse-score/devcontainer:latest

# RENAME 'vscode' user to 'bot6abt' to satisfy the QNX license
# We use usermod to change the login name and home directory
RUN usermod -l bot6abt vscode \
    && groupmod -n bot6abt vscode \
    && usermod -d /home/bot6abt -m bot6abt

# Optional: Fix sudoers if necessary (usually handled by group permissions, but good safety)
RUN echo "bot6abt ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/bot6abt \
    && chmod 0440 /etc/sudoers.d/bot6abt
